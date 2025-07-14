#include "run.h"

lw_context_t lw_ctx = {0};

void index_handler(http_request_t *req, http_response_t *res) {
    (void)req;
    render_html(res, "index.html");
}

int main(int argc, char *argv[]) {
    
    parameter_controller(argc, argv);

    printf("[LW] Starting Lower Web Framework...\n");
    printf("[LW] HTML Handler System\n");
    
    use_static_files();

    lw_route(GET, "/", index_handler);

    printf("[LW] Routes registered successfully!\n");
    printf("[LW] HTML files will be loaded from ./public/html/\n");
    printf("[LW] Static files will be loaded from ./public/\n");

    return lw_run(LW_PORT);
}
