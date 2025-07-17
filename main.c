#include "run.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <pthread.h>

void start_redirector(void);

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
    } else if (LW_SSL_ENABLED == 1) {
        init_openssl();
        ssl_ctx = create_ssl_ctx();
        configure_ssl_ctx(ssl_ctx, LW_CERT_FILE, LW_KEY_FILE);
    }

    printf("[LW] Starting Lower Web Framework...\n");
    printf("[LW] HTML Handler System\n");
    
    use_static_files();

    lw_route(GET, "/", index_handler);

    printf("[LW] Routes registered successfully!\n");
    printf("[LW] HTML files will be loaded from ./public/html/\n");
    printf("[LW] Static files will be loaded from ./public/\n");

    if (LW_SSL_ENABLED == 1) start_redirector(); 
    /* Fun fact : While i was doing TLS/SSL support, i remembered that my mom brought me cotton candy.
     * While i finished the hard parts, i started to eat it, i finished it. Then my momma brought me ayran(Turkish drink).
     * IT EVEN HAS FOAMS. LOVE MY MOMMA */

    return lw_run(LW_PORT);
}
