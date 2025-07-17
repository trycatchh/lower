#include "https.h"
#include <openssl/err.h>

static SSL_CTX *g_ctx = NULL;

int https_init(const char *crt_file, const char *key_file)
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    g_ctx = SSL_CTX_new(TLS_server_method());
    if (!g_ctx) goto err;

    if (SSL_CTX_use_certificate_file(g_ctx, crt_file, SSL_FILETYPE_PEM) <= 0)
        goto err;
    if (SSL_CTX_use_PrivateKey_file(g_ctx, key_file, SSL_FILETYPE_PEM) <= 0)
        goto err;

    return 0;
err:
    ERR_print_errors_fp(stderr);
    return -1;
}

void https_cleanup(void)
{
    if (g_ctx) SSL_CTX_free(g_ctx);
    EVP_cleanup();
}

SSL *https_new_ssl(int plain_socket)
{
    SSL *ssl = SSL_new(g_ctx);
    if (!ssl) return NULL;
    SSL_set_fd(ssl, plain_socket);
    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}

void https_close_ssl(SSL *ssl)
{
    if (!ssl) return;
    SSL_shutdown(ssl);
    SSL_free(ssl);
}
