#ifndef HTTPS_H
#define HTTPS_H

#include <openssl/ssl.h>

// lifetime helpers
int  https_init(const char *crt_file, const char *key_file);
void https_cleanup(void);

// per-connection helpers
SSL *https_new_ssl(int plain_socket);
void https_close_ssl(SSL *ssl);

#endif
