#ifndef RUN_H
#define RUN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_HEADERS 50
#define MAX_ROUTES 100
#define BUFFER_SIZE 4096
#define MAX_PATH_LENGTH 256

// HTTP method enumeration
typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    UNKNOWN
} http_method_t;

// HTTP request structure
typedef struct {
    http_method_t method;
    char *path;
    char *query_string;
    char *headers[MAX_HEADERS];
    int header_count;
    char *body;
    size_t body_length;
    void *user_data; 
} http_request_t;

// HTTP response structure
typedef struct {
    int status_code;
    char *headers[MAX_HEADERS];
    int header_count;
    char *body;
    size_t body_length; 
} http_response_t;

// Route handler function pointer
typedef void (*route_handler_t)(http_request_t *request, http_response_t *response);

// Route structure
typedef struct {
    http_method_t method;
    char path[MAX_PATH_LENGTH];
    route_handler_t handler;
} route_t;

// Framework context
typedef struct {
    route_t routes[MAX_ROUTES];
    int route_count;
    int server_fd;
    int port;
} lw_context_t;

// Global context
extern lw_context_t lw_ctx;

// Core functions
int lw_run(int port);
void lw_route(http_method_t method, const char *path, route_handler_t handler);
void lw_send_response(http_response_t *response, int client_socket);
void lw_set_header(http_response_t *response, const char *header);
void lw_set_body(http_response_t *response, const char *body);

// HTTP parsing functions
http_method_t parse_method(const char *method_str);
void parse_request(const char *raw_request, http_request_t *request);
void free_request(http_request_t *request);
void init_response(http_response_t *response);
void free_response(http_response_t *response);

// Helper functions
const char *method_to_string(http_method_t method);
route_t *find_route(http_method_t method, const char *path);

#endif // RUN_H
