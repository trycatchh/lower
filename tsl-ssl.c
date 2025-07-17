#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>

X509 *load_cert_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);
    fclose(fp);
    return cert;
}

EVP_PKEY *load_private_key(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    return pkey;
}

/*
if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 || SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
}
*/
void dump_cert_info(const X509 *cert) {
    char buf[256];

    X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
    printf("Subject : %s\n", buf);

    X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
    printf("Issuer : %s\n", buf);
}

X509 *create_self_signed(EVP_PKEY **pkey_out, int bits /* 2048/4096 */ ) {
    EVP_PKEY *pkey = EVP_RSA_gen(bits); // generate RSA key
    if (!pkey) return NULL;

    X509 *x = X509_new();
    ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
    X509_gmtime_adj(X509_get_notBefore(x), 0); // Now
    X509_gmtime_adj(X509_get_notAfter(x), 31536000L); // +1 year
    
    X509_NAME *name = X509_NAME_new();
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);
    X509_set_issuer_name(x, name);
    X509_set_subject_name(x, name);
    X509_NAME_free(name);

    X509_set_pubkey(x, pkey);
    X509_sign(x, pkey, EVP_sha256());

    *pkey_out = pkey;
    return x; // caller must be free!
}

void write_cert_to_pem(const X509 *cert, const char *path) {
    FILE *fp = fopen(path, "w");
    PEM_write_X509(fp, cert);
    fclose(fp);
}

void write_pkey_to_pem(const EVP_PKEY *pkey, const char *path) {
    FILE *fp = fopen(path, "w");
    PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(fp);
}

void clean_ssl(const X509 *cert, const EVP_PKEY *pkey) {
    X509_free(cert);
    EVP_PKEY_free(pkey);
    SSL_CTX_free(ctx);
    EVP_cleanup();
}
