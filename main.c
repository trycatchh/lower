#include "run.h"

void hello_handler(http_request_t *req, http_response_t *res) {
    lw_set_header(res, "Content-Type: text/plain");
    lw_set_body(res, "Hello from Lower Web Framework!");
}

void json_handler(http_request_t *req, http_response_t *res) {
    lw_set_header(res, "Content-Type: application/json");
    lw_set_body(res, "{\"message\": \"Hello JSON!\", \"framework\": \"Lower Web Framework\"}");
}

void about_handler(http_request_t *req, http_response_t *res) {
    lw_set_header(res, "Content-Type: text/html");
    lw_set_body(res, "<h1>About Lower Web Framework</h1><p>A simple C web framework</p>");
}

int main() {
    printf("[LW] Starting Lower Web Framework...\n");
    
    lw_route(GET, "/", hello_handler);
    lw_route(GET, "/hello", hello_handler);
    lw_route(GET, "/json", json_handler);
    lw_route(GET, "/about", about_handler);
    
    return lw_run(8080);
}