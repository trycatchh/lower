#include "run.h"

http_method_t parse_method(const char *method_str) {
    if (strncmp(method_str, "GET", 3) == 0) return GET;
    if (strncmp(method_str, "POST", 4) == 0) return POST;
    if (strncmp(method_str, "PUT", 3) == 0) return PUT;
    if (strncmp(method_str, "DELETE", 6) == 0) return DELETE;
    if (strncmp(method_str, "PATCH", 5) == 0) return PATCH;
    if (strncmp(method_str, "HEAD", 4) == 0) return HEAD;
    if (strncmp(method_str, "OPTIONS", 7) == 0) return OPTIONS;
    return UNKNOWN;
}

void parse_request(const char *raw_request, http_request_t *request) {
    char *request_copy = malloc(strlen(raw_request) + 1);
    strcpy(request_copy, raw_request);
    
    // Parse request line
    char *line = strtok(request_copy, "\r\n");
    if (line) {
        char *method_str = strtok(line, " ");
        char *path_str = strtok(NULL, " ");
        
        if (method_str) {
            request->method = parse_method(method_str);
        }
        
        if (path_str) {
            // Check for query string
            char *query_start = strchr(path_str, '?');
            if (query_start) {
                *query_start = '\0';
                query_start++;
                request->query_string = malloc(strlen(query_start) + 1);
                strcpy(request->query_string, query_start);
            }
            
            request->path = malloc(strlen(path_str) + 1);
            strcpy(request->path, path_str);
        }
    }
    
    // Parse headers
    request->header_count = 0;
    while ((line = strtok(NULL, "\r\n")) && strlen(line) > 0 && request->header_count < MAX_HEADERS) {
        request->headers[request->header_count] = malloc(strlen(line) + 1);
        strcpy(request->headers[request->header_count], line);
        request->header_count++;
    }
    
    // Parse body
    if (line && (line = strtok(NULL, "\0"))) {
        request->body_length = strlen(line);
        request->body = malloc(request->body_length + 1);
        strcpy(request->body, line);
    }
    
    free(request_copy);
}

void free_request(http_request_t *request) {
    if (request->path) free(request->path);
    if (request->query_string) free(request->query_string);
    if (request->body) free(request->body);
    
    for (int i = 0; i < request->header_count; i++) {
        if (request->headers[i]) free(request->headers[i]);
    }
}

void init_response(http_response_t *response) {
    response->status_code = 200;
    response->header_count = 0;
    response->body = NULL;
    response->body_length = 0;
}

void free_response(http_response_t *response) {
    if (response->body) free(response->body);
    
    for (int i = 0; i < response->header_count; i++) {
        if (response->headers[i]) free(response->headers[i]);
    }
}
