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

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        res->status_code = 404;
        lw_set_header(res, "Content-Type: text/plain");
        lw_set_body(res, "404 - Static file not found");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);

    if (strstr(req->path, ".css") != NULL) {
        lw_set_header(res, "Content-Type: text/css");
    } else if (strstr(req->path, ".js") != NULL) {
        lw_set_header(res, "Content-Type: application/javascript");
    } else if (strstr(req->path, ".png") != NULL) {
        lw_set_header(res, "Content-Type: image/png");
    } else if (strstr(req->path, ".jpg") != NULL || strstr(req->path, ".jpeg") != NULL) {
        lw_set_header(res, "Content-Type: image/jpeg");
    } else {
        lw_set_header(res, "Content-Type: application/octet-stream");
    }

    lw_set_body(res, content);
    free(content);
}