#include "run.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>

lw_context_t lw_ctx = {0};

void index_handler(http_request_t *req, http_response_t *res) {
    (void)req;
    render_html(res, "index.html");
}

int main(int argc, char *argv[]) {
    
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    parameter_controller(argc, argv);

    if (LW_DEV_MODE) {
        start_live_reload_server(8181, "./public");
    }

    printf("[LW] Starting Lower Web Framework...\n");
    printf("[LW] HTML Handler System\n");
    
    use_static_files();

    lw_route(GET, "/", index_handler);

    printf("[LW] Routes registered successfully!\n");
    printf("[LW] HTML files will be loaded from ./public/html/\n");
    printf("[LW] Static files will be loaded from ./public/\n");

    return lw_run(LW_PORT);
}
