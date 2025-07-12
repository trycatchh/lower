#include "run.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int lw_run(int PORT) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    const char *http_response = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: 13\r\n"
                                "\r\n"
                                "Hello, World!";
    
    /* Create Socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[ERR] Socket not created");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /* Listening Mode */
    if (listen(server_fd, 10) < 0) {
        perror("[ERR] Listen failed");
        close(server_fd);
        return -1;
    }

    printf("[SM] Server is listening on port %d\n", PORT);
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0) {
            perror("[ERR] Accept failed");
            continue;
        }
        read(new_socket, buffer, BUFFER_SIZE - 1);
        print("[SM] Incoming request:\n%s\n", buffer);

        write(new_socket, http_response, strlen(http_response));

        close(new_socket);
    }

    close(server_fd);
    return 0;
}