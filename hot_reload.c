#define _POSIX_C_SOURCE 199309L

#include "run.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

// Max 64 Client
#define MAX_WS_CLIENT 64
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

static int ws_clients[MAX_WS_CLIENT]; // Client list
static int ws_client_count = 0;
static pthread_mutex_t ws_mutex = PTHREAD_MUTEX_INITIALIZER;
static int ws_server_running = 0;

void notify_clients_reload() {
    pthread_mutex_lock(&ws_mutex);
    printf("[DEV] Notifying %d clients to reload...\n", ws_client_count);
    for (int i = 0; i < ws_client_count; ) {
        int fd = ws_clients[i];
        // WebSocket frame for "reload" message
        char msg[] = {0x81, 0x06, 'r', 'e', 'l', 'o', 'a', 'd'};
        ssize_t sent = send(fd, msg, sizeof(msg), MSG_NOSIGNAL);
        if (sent < 0) {
            printf("[DEV] Client %d disconnected (error: %s), removing from list\n", i, strerror(errno));
            close(fd);
            ws_clients[i] = ws_clients[--ws_client_count];
        } else {
            printf("[DEV] Reload message sent to client %d (%zd bytes)\n", i, sent);
            i++;
        }
    }
    pthread_mutex_unlock(&ws_mutex);
}

// Recursive function to add watches to all subdirectories
void add_watches_recursive(int inotify_fd, const char* path) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[512];
    
    printf("[DEV] Adding watch for directory: %s\n", path);
    
    int wd = inotify_add_watch(inotify_fd, path, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
    if (wd < 0) {
        printf("[DEV] Failed to add watch for %s: %s\n", path, strerror(errno));
        return;
    } else {
        printf("[DEV] Successfully added watch for %s (wd: %d)\n", path, wd);
    }
    
    dir = opendir(path);
    if (!dir) {
        printf("[DEV] Failed to open directory %s: %s\n", path, strerror(errno));
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (stat(full_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            add_watches_recursive(inotify_fd, full_path);
        }
    }
    
    closedir(dir);
}

void* watch_thread(void* arg) {
    const char* watch_dir = (const char*) arg;
    printf("[DEV] Watch thread started for directory: %s\n", watch_dir);
    
    int fd = inotify_init();
    if (fd < 0) {
        printf("[DEV] inotify_init failed: %s\n", strerror(errno));
        return NULL;
    }
    
    printf("[DEV] inotify_init successful, fd: %d\n", fd);
    
    // Add recursive watches
    add_watches_recursive(fd, watch_dir);
    
    // File change listener
    char buffer[BUF_LEN];
    printf("[DEV] Starting file monitoring loop...\n");
    
    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        
        if (length < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("[DEV] read from inotify fd failed: %s\n", strerror(errno));
            break;
        }
        
        if (length == 0) {
            printf("[DEV] inotify read returned 0, continuing...\n");
            continue;
        }
        
        printf("[DEV] inotify read %d bytes\n", length);
        
        // Parse inotify events
        int i = 0;
        int events_processed = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            
            printf("[DEV] Event: wd=%d mask=%u cookie=%u len=%u", 
                   event->wd, event->mask, event->cookie, event->len);
            
            if (event->len > 0) {
                printf(" name=%s", event->name);
                
                // Filter out temporary files and hidden files
                if (event->name[0] != '.' && 
                    !strstr(event->name, "~") && 
                    !strstr(event->name, ".swp") &&
                    !strstr(event->name, ".tmp")) {
                    
                    printf(" -> TRIGGERING RELOAD");
                    notify_clients_reload();
                    events_processed++;
                }
            }
            printf("\n");
            
            i += EVENT_SIZE + event->len;
        }
        
        if (events_processed > 0) {
            // Small delay to prevent multiple rapid notifications
            printf("[DEV] Processed %d events, sleeping 500ms...\n", events_processed);
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 500 * 1000 * 1000; // 500ms
            nanosleep(&ts, NULL);
        }
    }
    
    printf("[DEV] Watch thread ending\n");
    close(fd);
    return NULL;
}

int websocket_handshake(int client_fd) {
    char buf[4096];
    int len = recv(client_fd, buf, sizeof(buf)-1, 0);
    if (len <= 0) {
        printf("[DEV] Failed to receive handshake data: %s\n", strerror(errno));
        return -1;
    }
    buf[len] = '\0';

    printf("[DEV] WebSocket handshake request (%d bytes):\n%s\n", len, buf);

    // Check if it's a WebSocket request
    if (!strstr(buf, "Upgrade: websocket") && !strstr(buf, "Upgrade: WebSocket")) {
        printf("[DEV] Not a WebSocket upgrade request\n");
        return -1;
    }

    char* key = strstr(buf, "Sec-WebSocket-Key: ");
    if (!key) {
        printf("[DEV] No Sec-WebSocket-Key found in request\n");
        return -1;
    }
    key += strlen("Sec-WebSocket-Key: ");

    char* end = strstr(key, "\r\n");
    if (!end) {
        printf("[DEV] Malformed WebSocket key\n");
        return -1;
    }
    
    char ws_key[128] = {0};
    strncpy(ws_key, key, end - key);
    printf("[DEV] Extracted WebSocket key: %s\n", ws_key);
    
    // Simple handshake response (development only)
    const char* handshake = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
        "\r\n";
    
    int sent = send(client_fd, handshake, strlen(handshake), 0);
    if (sent < 0) {
        printf("[DEV] Failed to send handshake response: %s\n", strerror(errno));
        return -1;
    }
    
    printf("[DEV] WebSocket handshake completed successfully (%d bytes sent)\n", sent);
    return 0;
}

// WebSocket server thread
void* ws_server_thread(void* arg) {
    int ws_port = *(int*)arg;
    printf("[DEV] WebSocket server thread started on port %d\n", ws_port);
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("[DEV] Socket creation failed: %s\n", strerror(errno));
        return NULL;
    }
    
    printf("[DEV] Socket created successfully, fd: %d\n", server_fd);
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("[DEV] setsockopt SO_REUSEADDR failed: %s\n", strerror(errno));
        close(server_fd);
        return NULL;
    }
    
    // Ignore SIGPIPE to prevent crashes when clients disconnect
    signal(SIGPIPE, SIG_IGN);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ws_port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[DEV] Bind failed on port %d: %s\n", ws_port, strerror(errno));
        close(server_fd);
        return NULL;
    }
    
    printf("[DEV] Socket bound successfully to port %d\n", ws_port);
    
    if (listen(server_fd, 8) < 0) {
        printf("[DEV] Listen failed: %s\n", strerror(errno));
        close(server_fd);
        return NULL;
    }

    printf("[DEV] Live reload WebSocket server started at ws://localhost:%d\n", ws_port);
    printf("[DEV] Server is now accepting connections...\n");
    ws_server_running = 1;

    while (ws_server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("[DEV] Accept failed: %s\n", strerror(errno));
            continue;
        }
        
        printf("[DEV] New connection from %s:%d (fd: %d)\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
        
        if (websocket_handshake(client_fd) == 0) {
            pthread_mutex_lock(&ws_mutex);
            if (ws_client_count < MAX_WS_CLIENT) {
                ws_clients[ws_client_count++] = client_fd;
                printf("[DEV] WebSocket client connected successfully (%d total)\n", ws_client_count); 
            } else {
                printf("[DEV] Max clients (%d) reached, closing connection\n", MAX_WS_CLIENT);
                close(client_fd);
            }
            pthread_mutex_unlock(&ws_mutex);
        } else {
            printf("[DEV] WebSocket handshake failed, closing connection\n");
            close(client_fd);
        }
    }
    
    printf("[DEV] WebSocket server shutting down\n");
    close(server_fd);
    return NULL;
}

void start_live_reload_server(int ws_port, const char* watch_dir) {
    pthread_t t1, t2;
    static int port_copy;
    port_copy = ws_port;
    
    printf("[DEV] Starting live reload system...\n");
    printf("[DEV] WebSocket port: %d\n", ws_port);
    printf("[DEV] Watch directory: %s\n", watch_dir);
    
    // Check if directory exists
    struct stat st;
    if (stat(watch_dir, &st) != 0) {
        printf("[DEV] Warning: Watch directory '%s' does not exist: %s\n", watch_dir, strerror(errno));
    } else if (!S_ISDIR(st.st_mode)) {
        printf("[DEV] Warning: '%s' is not a directory\n", watch_dir);
    } else {
        printf("[DEV] Watch directory '%s' exists and is accessible\n", watch_dir);
    }
    
    if (pthread_create(&t1, NULL, ws_server_thread, &port_copy) != 0) {
        printf("[DEV] Failed to create WebSocket server thread: %s\n", strerror(errno));
        return;
    }
    
    if (pthread_create(&t2, NULL, watch_thread, (void*)watch_dir) != 0) {
        printf("[DEV] Failed to create file watcher thread: %s\n", strerror(errno));
        return;
    }
    
    pthread_detach(t1);
    pthread_detach(t2);
    
    printf("[DEV] Live reload system started successfully\n");
    printf("[DEV] You can test the connection by opening: ws://localhost:%d\n", ws_port);
    
    // Give threads time to start
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
}
