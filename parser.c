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
    const char *current = raw_request;
    const char *line_end;
    char line_buffer[2048];
    
    // Initialize request
    memset(request, 0, sizeof(*request));
    
    // Parse request line (first line)
    line_end = strstr(current, "\r\n");
    if (!line_end) return;
    
    size_t line_len = line_end - current;
    if (line_len >= sizeof(line_buffer)) line_len = sizeof(line_buffer) - 1;
    strncpy(line_buffer, current, line_len);
    line_buffer[line_len] = '\0';
    
    // Parse method, path, and version from request line
    char *method_str = strtok(line_buffer, " ");
    char *path_str = strtok(NULL, " ");
    char *version_str = strtok(NULL, " ");
    
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
    
    // Move to next line
    current = line_end + 2; // Skip \r\n
    
    // Parse headers
    request->header_count = 0;
    while (*current && request->header_count < MAX_HEADERS) {
        line_end = strstr(current, "\r\n");
        if (!line_end) break;
        
        line_len = line_end - current;
        
        // Empty line indicates end of headers
        if (line_len == 0) {
            current = line_end + 2; // Skip \r\n
            break;
        }
        
        // Copy header line
        if (line_len >= sizeof(line_buffer)) line_len = sizeof(line_buffer) - 1;
        strncpy(line_buffer, current, line_len);
        line_buffer[line_len] = '\0';
        
        // Check if it's a valid header (contains colon)
        if (strchr(line_buffer, ':')) {
            request->headers[request->header_count] = malloc(strlen(line_buffer) + 1);
            strcpy(request->headers[request->header_count], line_buffer);
            request->header_count++;
        }
        
        // Move to next line
        current = line_end + 2; // Skip \r\n
    }
    
    // Parse body (if any)
    if (*current) {
        size_t body_len = strlen(current);
        if (body_len > 0) {
            request->body_length = body_len;
            request->body = malloc(body_len + 1);
            strcpy(request->body, current);
        }
    }
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
