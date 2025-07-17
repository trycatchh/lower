#include "run.h"

void lw_route(http_method_t method, const char *path, route_handler_t handler) {
    if (lw_ctx.route_count >= MAX_ROUTES) {
        fprintf(stderr, "[ERR] Maximum number of routes exceeded\n");
        return;
    }

    route_t *route = &lw_ctx.routes[lw_ctx.route_count];
    route->method = method;
    strncpy(route->path, path, MAX_PATH_LENGTH - 1);
    route->path[MAX_PATH_LENGTH - 1] = '\0';
    route->handler = handler;
    
    lw_ctx.route_count++;
    
    LW_VERBOSE ? printf("[LW] Route registered: %s %s\n", method_to_string(method), path) : 0;
}

void lw_send_response(http_response_t *response, int client_socket, SSL *client_ssl) {
    char response_buffer[BUFFER_SIZE * 2];
    int offset = 0;
    
    // Status line
    offset += snprintf(response_buffer + offset, sizeof(response_buffer) - offset, 
                      "HTTP/1.1 %d %s\r\n", response->status_code, 
                      response->status_code == 200 ? "OK" : 
                      response->status_code == 404 ? "Not Found" : 
                      response->status_code == 500 ? "Internal Server Error" : "Unknown");
    
    // Headers
    for (int i = 0; i < response->header_count; i++) {
        offset += snprintf(response_buffer + offset, sizeof(response_buffer) - offset, 
                          "%s\r\n", response->headers[i]);
    }
    
    // Content-Length header
    if (response->body && response->body_length > 0) {
        offset += snprintf(response_buffer + offset, sizeof(response_buffer) - offset, 
                          "Content-Length: %zu\r\n", response->body_length);
    }
    
    // End of headers
    offset += snprintf(response_buffer + offset, sizeof(response_buffer) - offset, "\r\n");
    
    // Body
    if (response->body && response->body_length > 0) {
        memcpy(response_buffer + offset, response->body, response->body_length);
        offset += response->body_length;
    }
    
    // Send response - use SSL if available, otherwise regular socket
    if (LW_SSL_ENABLED == 1 && client_ssl) {
        SSL_write(client_ssl, response_buffer, offset);
    } else {
        write(client_socket, response_buffer, offset);
    }
}

void lw_set_header(http_response_t *response, const char *header) {
    if (response->header_count >= MAX_HEADERS) {
        fprintf(stderr, "[ERR] Maximum number of headers exceeded\n");
        return;
    }
    
    response->headers[response->header_count] = malloc(strlen(header) + 1);
    strcpy(response->headers[response->header_count], header);
    response->header_count++;
}

void lw_set_body(http_response_t *response, const char *body) {
    if (response->body) {
        free(response->body);
    }
    
    response->body_length = strlen(body);
    response->body = malloc(response->body_length + 1);
    strcpy(response->body, body);
}

void lw_set_body_bin(http_response_t *response, const char *body, size_t length) {
    if (response->body) free(response->body);
    response->body_length = length;
    response->body = malloc(length);
    memcpy(response->body, body, length);
}

