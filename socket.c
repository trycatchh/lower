#include "run.h"

void use_static_files();

int lw_run(int port) {    
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    lw_ctx.port = port;
    
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

    printf("[LW] Server is listening on http://localhost:%d\n", port);
    
    while (1) {
        int client_socket = accept(lw_ctx.server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
        if (client_socket < 0) {
            perror("[ERR] Accept failed");
            continue;
        }

        // Read request
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("[ERR] Read failed");
            close(client_socket);
            continue;
        }
        
        char ipstring[INET_ADDRSTRLEN];
        getpeername(lw_ctx.server_fd, (struct sockaddr*)&address, (socklen_t *)&addr_len);
        struct sockaddr_in *s = (struct sockaddr_in *) &address;
        inet_ntop(AF_INET, &s->sin_addr, ipstring, sizeof(ipstring));

        if (strcmp(ipstring, "127.0.0.1")== 0) {
            addr_len = sizeof(addr_len);
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
            lw_send_response(&response, client_socket);
        }
        
        // Cleanup
        free_request(&request);
        free_response(&response);
        close(client_socket);
    }

    close(lw_ctx.server_fd);
    return 0;
}


