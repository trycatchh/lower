#include "run.h"
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define PORT 8443

// Initializing OpenSSL
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

// Creating SSL context(ctx)

SSL_CTX* create_ssl_ctx() {
    const SSL_METHOD *method;
    SSL_CTX* ctx;

    method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        printf("[ERR] Unable to create SSL context\n");
        ERR_print_errors_fp(stderr);
        printf("[LW] Exiting...\n");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_ssl_ctx(SSL_CTX *ctx, const char* cert_file, const char* key_file) {
    // Loading cert file.
    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        printf("[ERR] Unable to load certificate file\n");
        ERR_print_errors_fp(stderr);
        printf("[LW] Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // Loading private key file.
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        printf("[ERR] Unable to load private key file\n");
        ERR_print_errors_fp(stderr);
        printf("[LW] Exiting...\n");
        exit(EXIT_FAILURE);
    }

    // Verifying that certificate and private key matches.
    if (!SSL_CTX_check_private_key(ctx)) {
        printf("[ERR] Private key doesn't matches the public certificate key\n");
        // Not exiting, it will force it to use HTTP. (It just came to my mind btw :D)
    } else { 
        if (LW_DEV_MODE == 1) printf("[DEV] Private key and public certificate key matches.\n"); 
    }

    // Setting the security levels and cipher preferences.
    SSL_CTX_set_security_level(ctx, LW_SSL_SECLVL);
    SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!MD4");
}
