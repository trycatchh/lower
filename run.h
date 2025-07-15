#ifndef RUN_H
#define RUN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_HEADERS       50
#define MAX_ROUTES        100
#define BUFFER_SIZE       4096
#define MAX_PATH_LENGTH   256

// Global constants
extern int LW_PORT;
extern int LW_VERBOSE;
extern int LW_DEV_MODE;

// Enums & Structs
typedef enum {
    GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS, UNKNOWN
} http_method_t;

typedef struct {
    http_method_t method;
    char *path;
    char *query_string;
    char *headers[MAX_HEADERS];
    int   header_count;
    char *body;
    size_t body_length;
    void *user_data;
} http_request_t;

typedef struct {
    int   status_code;
    char *headers[MAX_HEADERS];
    int   header_count;
    char *body;
    size_t body_length;
    int   chunked_fd;   /* >=0 -> chunked stream */
} http_response_t;

typedef void (*route_handler_t)(http_request_t *, http_response_t *);

typedef struct {
    http_method_t method;
    char path[MAX_PATH_LENGTH];
    route_handler_t handler;
} route_t;

typedef struct {
    route_t routes[MAX_ROUTES];
    int route_count;
    int server_fd;
    int port;
} lw_context_t;

// Shared chunked-state
typedef struct {
    int chunked_fds[64];
    int chunked_count;
    pthread_mutex_t chunked_mutex;
} chunked_state_t;

extern chunked_state_t chunked_state;

// Global context
extern lw_context_t lw_ctx;

// Functions
int  lw_run(int port);
void lw_route(http_method_t method, const char *path, route_handler_t handler);
void lw_send_response(http_response_t *response, int client_socket);
void lw_set_header(http_response_t *response, const char *header);
void lw_set_body(http_response_t *response, const char *body);
void lw_set_body_bin(http_response_t *response, const char *body, size_t length);

http_method_t parse_method(const char *method_str);
void parse_request(const char *raw_request, http_request_t *request);
void free_request(http_request_t *request);
void init_response(http_response_t *response);
void free_response(http_response_t *response);

const char *method_to_string(http_method_t method);
route_t *find_route(http_method_t method, const char *path);

char *load_html_file(const char *filename);
void  render_html(http_response_t *res, const char *filename);
void  static_file_handler(http_request_t *req, http_response_t *res);
void  use_static_files(void);

int parameter_controller(int argc, char *argv[]);
void print_help(void);

void start_live_reload_server(int ws_port_unused, const char *watch_dir);

#endif
