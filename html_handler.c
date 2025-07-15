#define _GNU_SOURCE
#include "run.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

char* load_html_file(const char* filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "./public/html/%s", filename);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        printf("[ERR] HTML file not found: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, file),
    content[file_size] = '\0';

    fclose(file);
    printf("[LW] HTML file load: %s\n", filepath);
    return content;
}

static void chunked_write(int fd, const char *data, size_t len)
{
    char header[32];
    int hdr_len = snprintf(header, sizeof(header), "%zx\r\n", len);
    send(fd, header, hdr_len, MSG_NOSIGNAL);
    send(fd, data, len, MSG_NOSIGNAL);
    send(fd, "\r\n", 2, MSG_NOSIGNAL);
}

void render_html(http_response_t *res, const char *filename)
{
    if (!LW_DEV_MODE) {
        /* old behaviour */
        char *content = load_html_file(filename);
        if (!content) {
            res->status_code = 404;
            lw_set_header(res, "Content-Type: text/html; charset=utf-8");
            lw_set_body(res, "<h1>404 Not Found</h1>");
            return;
        }
        lw_set_header(res, "Content-Type: text/html; charset=utf-8");
        lw_set_body(res, content);
        free(content);
        return;
    }

    /* ---------- chunked streaming path ---------- */
    res->status_code = 200;
    lw_set_header(res, "Content-Type: text/html; charset=utf-8");
    lw_set_header(res, "Transfer-Encoding: chunked");
    lw_set_header(res, "Cache-Control: no-cache");
    lw_set_header(res, "Connection: close");

    /* send headers first */
    char header_buf[2048];
    int off = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n");
    send(res->chunked_fd, header_buf, off, MSG_NOSIGNAL);

    pthread_mutex_lock(&chunked_state.chunked_mutex);
    if (chunked_state.chunked_count < 64) {
        chunked_state.chunked_fds[chunked_state.chunked_count++] = res->chunked_fd;
        pthread_mutex_unlock(&chunked_state.chunked_mutex);
    }

    /* render & send initial chunk */
    char *content = load_html_file(filename);
    if (!content) content = strdup("<h1>404 Not Found</h1>");
    chunked_write(res->chunked_fd, content, strlen(content));
    free(content);
}

void static_file_handler(http_request_t *req, http_response_t *res) {
    char filepath[512];
    const char *base_path = "./public";

    if (strstr(req->path, "..") != NULL) {
        res->status_code = 403;
        lw_set_header(res, "Content-Type: text/plain");
        lw_set_body(res, "403 Forbidden");
        return;
    }

    snprintf(filepath, sizeof(filepath), "%s%s", base_path, req->path);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        res->status_code = 404;
        lw_set_header(res, "Content-Type: text/html");
        lw_set_body(res, "<h1>404 Not Found</h1>");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size);
    fread(content, 1, file_size, file);
    fclose(file);

    if (strstr(req->path, ".css")) {
        lw_set_header(res, "Content-Type: text/css");
    } else if (strstr(req->path, ".js")) {
        lw_set_header(res, "Content-Type: application/javascript");
    } else if (strstr(req->path, ".png")) {
        lw_set_header(res, "Content-Type: image/png");
    } else if (strstr(req->path, ".jpg") || strstr(req->path, ".jpeg")) {
        lw_set_header(res, "Content-Type: image/jpeg");
    } else if (strstr(req->path, ".gif")) {
        lw_set_header(res, "Content-Type: image/gif");
    } else if (strstr(req->path, ".svg")) {
        lw_set_header(res, "Content-Type: image/svg+xml");
    } else if (strstr(req->path, ".ico")) {
        lw_set_header(res, "Content-Type: image/x-icon");
    } else if (strstr(req->path, ".woff2")) {
        lw_set_header(res, "Content-Type: font/woff2");
    } else if (strstr(req->path, ".woff")) {
        lw_set_header(res, "Content-Type: font/woff");
    } else if (strstr(req->path, ".ttf")) {
        lw_set_header(res, "Content-Type: font/ttf");
    } else if (strstr(req->path, ".otf")) {
        lw_set_header(res, "Content-Type: font/otf");
    } else if (strstr(req->path, ".eot")) {
        lw_set_header(res, "Content-Type: application/vnd.ms-fontobject");
    } else if (strstr(req->path, ".json")) {
        lw_set_header(res, "Content-Type: application/json");
    } else if (strstr(req->path, ".xml")) {
        lw_set_header(res, "Content-Type: application/xml");
    } else if (strstr(req->path, ".pdf")) {
        lw_set_header(res, "Content-Type: application/pdf");
    } else if (strstr(req->path, ".zip")) {
        lw_set_header(res, "Content-Type: application/zip");
    } else if (strstr(req->path, ".txt")) {
        lw_set_header(res, "Content-Type: text/plain");
    } else if (strstr(req->path, ".html") || strstr(req->path, ".htm")) {
        lw_set_header(res, "Content-Type: text/html; charset=utf-8");
    } else {
        lw_set_header(res, "Content-Type: application/octet-stream");
    }

    lw_set_body_bin(res, content, file_size);
    free(content);
}

void use_static_files() {
    lw_route(GET, "/css/", static_file_handler);
    lw_route(GET, "/js/", static_file_handler);
    lw_route(GET, "/img/", static_file_handler);
    lw_route(GET, "/images/", static_file_handler);
    lw_route(GET, "/fonts/", static_file_handler);
    lw_route(GET, "/assets/", static_file_handler);
    lw_route(GET, "/uploads/", static_file_handler);
    lw_route(GET, "/media/", static_file_handler);
    lw_route(GET, "/favicon.ico", static_file_handler);
}
