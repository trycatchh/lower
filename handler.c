#include "run.h"
#include <strings.h>   /* strcasecmp */

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

void lw_send_response(http_response_t *response, int client_socket, SSL *client_ssl, const char *accept_encoding) {
    (LW_VERBOSE) ? printf("[COMP] LW_COMPRESS=%d  Accept-Encoding=%s  body=%zu\n",
       LW_COMPRESS, accept_encoding ? accept_encoding : "NULL", response->body_length) : 1;
    if (LW_COMPRESS &&
        response->body &&
        response->body_length > 0 &&
        accept_encoding &&
        strstr(accept_encoding, "zstd")) {

        size_t bound = ZSTD_compressBound(response->body_length);
        void  *zbuf  = malloc(bound);
        if (!zbuf) goto no_compress;

        size_t zlen = ZSTD_compress(zbuf, bound,
                                    response->body,
                                    response->body_length,
                                    3);                 // level 1-22
        if (ZSTD_isError(zlen)) {
            free(zbuf);
            goto no_compress;
        }

        char hdr[2048];
        int  hoff = snprintf(hdr, sizeof(hdr),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Encoding: zstd\r\n"
                             "Content-Length: %zu\r\n",
                             response->status_code,
                             response->status_code == 200 ? "OK" :
                             response->status_code == 404 ? "Not Found" :
                             response->status_code == 500 ? "Internal Server Error" :
                             "Unknown",
                             zlen);

        for (int i = 0; i < response->header_count; ++i)
            hoff += snprintf(hdr + hoff, sizeof(hdr) - hoff,
                             "%s\r\n", response->headers[i]);
        hoff += snprintf(hdr + hoff, sizeof(hdr) - hoff, "\r\n");

        if (LW_SSL_ENABLED && client_ssl) {
            SSL_write(client_ssl, hdr, hoff);
            SSL_write(client_ssl, zbuf, zlen);
        } else {
            write(client_socket, hdr, hoff);
            write(client_socket, zbuf, zlen);
        }

        free(zbuf);
        return;
    }

no_compress:
    char response_buffer[BUFFER_SIZE * 2];
    int offset = 0;

    offset += snprintf(response_buffer + offset,
                       sizeof(response_buffer) - offset,
                       "HTTP/1.1 %d %s\r\n",
                       response->status_code,
                       response->status_code == 200 ? "OK" :
                       response->status_code == 404 ? "Not Found" :
                       response->status_code == 500 ? "Internal Server Error" :
                       "Unknown");

    // Headers
    for (int i = 0; i < response->header_count; ++i)
        offset += snprintf(response_buffer + offset,
                           sizeof(response_buffer) - offset,
                           "%s\r\n", response->headers[i]);

    // Content-Length (uncompressed)
    if (response->body && response->body_length > 0)
        offset += snprintf(response_buffer + offset,
                           sizeof(response_buffer) - offset,
                           "Content-Length: %zu\r\n",
                           response->body_length);

    offset += snprintf(response_buffer + offset,
                       sizeof(response_buffer) - offset,
                       "\r\n");

    // Body
    if (response->body && response->body_length > 0) {
        memcpy(response_buffer + offset,
               response->body,
               response->body_length);
        offset += response->body_length;
    }

    // Send
    if (LW_SSL_ENABLED && client_ssl)
        SSL_write(client_ssl, response_buffer, offset);
    else
        write(client_socket, response_buffer, offset);
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
    if (response->body)
        free(response->body);

    response->body_length = strlen(body);
    response->body = malloc(response->body_length + 1);
    strcpy(response->body, body);
}

void lw_set_body_bin(http_response_t *response, const char *body, size_t length) {
    if (response->body)
        free(response->body);

    response->body_length = length;
    response->body = malloc(length);
    memcpy(response->body, body, length);
}
