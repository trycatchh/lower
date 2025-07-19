#include "run.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>

static int http_redirect_port = 8080;

void use_static_files();
void start_redirector(void);

int lw_run(int port) {
 
    if (LW_SSL_ENABLED == 1) start_redirector(); 

    struct sockaddr_in address;
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    lw_ctx.port = port;
    
    // Initialize SSL if enabled
    if (LW_SSL_ENABLED == 1) {
        if (LW_CERT != 1 || LW_KEY != 1) {
            fprintf(stderr, "[ERR] SSL enabled but certificate or key not provided\n");
            return -1;
        }
        
        init_openssl();
        ssl_ctx = create_ssl_ctx();
        configure_ssl_ctx(ssl_ctx, LW_CERT_FILE, LW_KEY_FILE);
        
        printf("[LW] SSL/TLS enabled with certificate: %s\n", LW_CERT_FILE);
        printf("[LW] Server is listening on https://localhost:%d\n", port);
    } else {
        printf("[LW] Server is listening on http://localhost:%d\n", port);
    }
    
    // Create socket
    if ((lw_ctx.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[ERR] Socket creation failed");
        return -1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(lw_ctx.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("[ERR] Setsockopt failed");
        close(lw_ctx.server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind socket
    if (bind(lw_ctx.server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERR] Bind failed");
        close(lw_ctx.server_fd);
        return -1;
    }

    // Listen for connections
    if (listen(lw_ctx.server_fd, 10) < 0) {
        perror("[ERR] Listen failed");
        close(lw_ctx.server_fd);
        return -1;
    }
    
    while (1) {
        int client_socket = accept(lw_ctx.server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
        if (client_socket < 0) {
            perror("[ERR] Accept failed");
            continue;
        }

        SSL *client_ssl = NULL;
        
        // Handle SSL connection if enabled
        if (LW_SSL_ENABLED == 1) {
            client_ssl = SSL_new(ssl_ctx);
            if (!client_ssl) {
                fprintf(stderr, "[ERR] Failed to create SSL structure\n");
                close(client_socket);
                continue;
            }
            
            SSL_set_fd(client_ssl, client_socket);
            
            // Perform SSL handshake
            if (SSL_accept(client_ssl) <= 0) {
                fprintf(stderr, "[ERR] SSL handshake failed\n");
                ERR_print_errors_fp(stderr);
                SSL_free(client_ssl);
                close(client_socket);
                continue;
            }
        }

        // Read request
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read;
        
        if (LW_SSL_ENABLED == 1 && client_ssl) {
            bytes_read = SSL_read(client_ssl, buffer, BUFFER_SIZE - 1);
        } else {
            bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        }
        
        if (bytes_read < 0) {
            perror("[ERR] Read failed");
            if (client_ssl) {
                SSL_shutdown(client_ssl);
                SSL_free(client_ssl);
            }
            close(client_socket);
            continue;
        }
        
        // Get client IP
        char ipstring[INET_ADDRSTRLEN];
        getpeername(client_socket, (struct sockaddr*)&address, (socklen_t *)&addr_len);
        struct sockaddr_in *s = (struct sockaddr_in *) &address;
        inet_ntop(AF_INET, &s->sin_addr, ipstring, sizeof(ipstring));

        if (strcmp(ipstring, "127.0.0.1") == 0) {
            addr_len = sizeof(address);
            getsockname(client_socket, (struct sockaddr *)&address, (socklen_t *)&addr_len);
            inet_ntop(AF_INET, &s->sin_addr, ipstring, sizeof(ipstring));
        }

        LW_VERBOSE ? printf("[LW] Incoming request:\nIP: %s\n%s\n", ipstring, buffer) : printf("[LW] Incoming request: IP: %s\n", ipstring);

        // Parse request
        http_request_t request = {0};
        parse_request(buffer, &request);

        // Find route
        route_t *route = find_route(request.method, request.path);
        
        http_response_t response = {0};
        init_response(&response);
        response.chunked_fd = -1; // -1 for the default.

        if (route) {
            // Call route handler
            route->handler(&request, &response);
        } else {
            // 404 Not Found
            response.status_code = 404;
            lw_set_header(&response, "Content-Type: text/plain");
            lw_set_body(&response, "404 Not Found");
        }

        // Send response
        if (response.chunked_fd >= 0) {
            // It's already handled in 'render_html', so.. nothing to do here i guess ?
        } else {
            lw_send_response(&response, client_socket, client_ssl);
        }
        
        // Cleanup
        free_request(&request);
        free_response(&response);
        
        if (client_ssl) {
            SSL_shutdown(client_ssl);
            SSL_free(client_ssl);
        }
        close(client_socket);
    }

    close(lw_ctx.server_fd);
    
    if (LW_SSL_ENABLED == 1) {
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
    }
    
    return 0;
}

static void* redirect_worker(void *arg) {
    (void)arg; // To be honest, i don't even know what this does, only way to make this function work is using this thing.
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("[ERR] Redirect socket failed.\n"); return NULL; }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(http_redirect_port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[ERR] Redirect binding failed.\n");
        close(sock);
        return NULL;
    }

    if (listen(sock, 10) < 0) {
        perror("[ERR] Redirect listen failed.\n");
        close(sock);
        return NULL;
    }

    printf("[REDIRECT] Redirect listener on http://localhost:%d\n", http_redirect_port);

    while (1) {
        int client = accept(sock, NULL, NULL);
        if (client < 0) { perror("[ERR] Redirect accept failed.\n"); continue; };

        const char *rsp =
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://localhost:%d/\r\n"
            "Connection: close\r\n\r\n";
        char buf[256];
        int len = snprintf(buf, sizeof(buf), rsp, lw_ctx.port);
        write(client, buf, len);
        close(client);
    }

    // Fun fact : this situation is never reached, cuz u have a while (1) loop xD
    return NULL;
}

/* This section is about '__attribute__((constructor))' thing in the bottom. 
 * It's a trick that only GCC/Clang supports. 
 * Basically you tell the compiler to run this function automatically before main function starts. 
 * If you want to use it, uncomment it. If you don't want to use it do the step below.
 * STEP 1 : Add these code blocks to main.c, right before 'return lw_run()'
 * ```
 * if (LW_SSL_ENABLED == 1) {
 *   start_redirector();  
 * }
 * ```

// __attribute__((constructor))
static void start_redirector(void) {
    if (LW_SSL_ENABLED != 1) return; // Only when TLS is on.
    pthread_t tid;
    pthread_create(&tid, NULL, redirect_worker, NULL);
    pthread_detach(tid);
}*/

void start_redirector(void) {
    if (LW_SSL_ENABLED != 1) return; // Only when TLS is on.
    pthread_t tid;
    pthread_create(&tid, NULL, redirect_worker, NULL);
    pthread_detach(tid);
}

/* socket.c 19/07/2025 Review
 * Ok so, there's nothing to really be changed i guess ?
 * I/we need to only edit the HTTP redirector.
 * Soo, what i am/we are going to do is basically, it will redirect all HTTP ports to HTTPS.
 * @p0unter said that if you capture the users port, then you can redirect it.
 * But there's a little problem within that, in socket programming, when you get the clients port address, it's not the real one. (Actually it is, but it's complicated)
 * So as an example, a user is connecting to 'http://localhost:8080' we know that the user send a request to 8080 right ? 
 * Yes, the client sends a request to 8080 port, but the clients file descriptor is not in the 8080 port.
 * That's a big problem, because you can't directly get a clients port address, only the file descriptors port, and it's always random.
 * TODO : Fix the redirection issue (Multiple ports.)
*/
