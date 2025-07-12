#include "run.h"

const char *method_to_string(http_method_t method) {
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case PUT: return "PUT";
        case DELETE: return "DELETE";
        case PATCH: return "PATCH";
        case HEAD: return "HEAD";
        case OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

route_t *find_route(http_method_t method, const char *path) {
    for (int i = 0; i < lw_ctx.route_count; i++) {
        if (lw_ctx.routes[i].method == method && 
            strcmp(lw_ctx.routes[i].path, path) == 0) {
            return &lw_ctx.routes[i];
        }
    }
    return NULL;
}