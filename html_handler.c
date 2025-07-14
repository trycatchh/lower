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
    fread(content, 1, file_size, file);
    content[file_size] = '\0';

    fclose(file);
    printf("[LW] HTML file load: %s\n", filepath);
    return content;
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

extern int LW_DEV_MODE;
extern int LW_PORT;

void render_html(http_response_t *res, const char* filename) {
    char* content = load_html_file(filename);
    if (content == NULL) {
        res->status_code = 404;
        lw_set_header(res, "Content-Type: text/html; charset=utf-8");
        lw_set_body(res, "<!DOCTYPE html><html><head><title>404</title></head><body>"
                         "<h1>404 - Page Not Found</h1>"
                         "<p>This page not available.</p>"
                        "</body></html>");
        return;            
    }

    if (LW_DEV_MODE) {
        char ws_script[2048]; // Increased buffer size
        
        snprintf(ws_script, sizeof(ws_script),
            "<script>"
            "(function() {"
            "    if (window.location.hostname === 'localhost' || "
            "        window.location.hostname === '127.0.0.1') {"
            "        const wsPort = %d;"
            "        const isSecure = window.location.protocol === 'https:';"
            "        const wsProtocol = isSecure ? 'wss://' : 'ws://';"
            "        const wsUrl = wsProtocol + '127.0.0.1:' + wsPort;"
            "        console.log('[DEV] Connecting to:', wsUrl);"
            "        let reconnectAttempts = 0;"
            "        let ws;"
            ""
            "        function connect() {"
            "            try {"
            "                ws = new WebSocket(wsUrl);"
            "                ws.onopen = function() {"
            "                    console.log('[DEV] WebSocket connected');"
            "                    reconnectAttempts = 0;"
            "                };"
            "                ws.onmessage = function(event) {"
            "                    if (event.data === 'reload') {"
            "                        console.log('[DEV] Reloading page...');"
            "                        window.location.reload(true);"
            "                    }"
            "                };"
            "                ws.onclose = function() {"
            "                    const delay = Math.min(3000, 1000 * Math.pow(2, reconnectAttempts));"
            "                    console.log('[DEV] WebSocket closed. Reconnecting in ' + delay + 'ms...');"
            "                    setTimeout(connect, delay);"
            "                    reconnectAttempts++;"
            "                };"
            "                ws.onerror = function(error) {"
            "                    console.error('[DEV] WebSocket error:', error);"
            "                    if (ws) ws.close();"
            "                };"
            "            } catch (e) {"
            "                console.error('[DEV] WebSocket exception:', e);"
            "            }"
            "        }"
            "        connect();"
            "    }"
            "})();"
            "</script>", LW_PORT + 1000);

        char* body_pos = strstr(content, "</body>");
        if (body_pos) {
            size_t prefix_len = body_pos - content;
            size_t suffix_len = strlen(body_pos);
            size_t script_len = strlen(ws_script);
            size_t total_len = prefix_len + script_len + suffix_len;
            
            char* new_content = malloc(total_len + 1);
            if (new_content == NULL) {
                printf("[ERR] Memory allocation failed for script injection\n");
                lw_set_header(res, "Content-Type: text/html; charset=utf-8");
                lw_set_body(res, content);
                free(content);
                return;
            }
            
            memcpy(new_content, content, prefix_len);
            memcpy(new_content + prefix_len, ws_script, script_len);
            memcpy(new_content + prefix_len + script_len, body_pos, suffix_len);
            new_content[total_len] = '\0';
            
            free(content);
            content = new_content;
        }
    }

    lw_set_header(res, "Content-Type: text/html; charset=utf-8");
    lw_set_body(res, content);
    free(content);
}