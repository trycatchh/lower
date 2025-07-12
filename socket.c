#include "run.h"

lw_context_t lw_ctx = {0};

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

    printf("[LW] Server is listening on port %d\n", port);
    
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

        printf("[LW] Incoming request:\n%s\n", buffer);

        // Parse request
        http_request_t request = {0};
        parse_request(buffer, &request);

        // Find route
        route_t *route = find_route(request.method, request.path);
        
        http_response_t response = {0};
        init_response(&response);

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
        lw_send_response(&response, client_socket);

        // Cleanup
        free_request(&request);
        free_response(&response);
        close(client_socket);
    }

    close(lw_ctx.server_fd);
    return 0;
}