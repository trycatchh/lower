#include "run.h"

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

void render_html(http_response_t *res, const char* filename) {
    char* content = load_html_file(filename);
    if (content == NULL) {
        res->status_code = 404;
        lw_set_header(res, "Content-Type: text/html; charset=utf-8");
        lw_set_body(res, "<!DOCTYPE html><html><head><title>404</title></head><body>"
                         "<h1>404 - Page Not Found</h1>"
                         "<p>This page not avaible.</p>"
                        "</body></html>");
        return;            
    }

    lw_set_header(res, "Content-Type: text/html; charset=utf-8");
    lw_set_body(res, content);
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