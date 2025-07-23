#include "run.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

chunked_state_t chunked_state = {
    .chunked_count = 0,
    .chunked_mutex = PTHREAD_MUTEX_INITIALIZER
};

const char *method_to_string(http_method_t method)
{
    switch (method)
    {
    case GET:
        return "GET";
    case POST:
        return "POST";
    case PUT:
        return "PUT";
    case DELETE:
        return "DELETE";
    case PATCH:
        return "PATCH";
    case HEAD:
        return "HEAD";
    case OPTIONS:
        return "OPTIONS";
    default:
        return "UNKNOWN";
    }
}

route_t *find_route(http_method_t method, const char *path)
{
    for (int i = 0; i < lw_ctx.route_count; i++)
    {
        route_t *route = &lw_ctx.routes[i];
        if (route->method == method)
        {
            size_t route_len = strlen(route->path);

            if (strncmp(route->path, path, route_len) == 0)
            {
                return route;
            }
        }
    }
    return NULL;
}

int match_option(const char *arg, const char *short_opt, const char *long_opt) {
    return (strcmp(arg, short_opt) == 0 || strcmp(arg, long_opt) == 0);
}

int parameter_controller(int argc, char *argv[])
{
    for (int i = 1;i < argc;i++) {
        if (match_option(argv[i], "-h", "--help")) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (match_option(argv[i], "-v", "--verbose")) {
            LW_VERBOSE = 1;
        } else if (match_option(argv[i], "-p", "--port")) {
            int temp_val = atoi(argv[++i]);
            
            if (i >= argc) {
                fprintf(stderr, "[ERR] %s requires a value\n", argv[--i]);
                return -1;
            } else if (temp_val > 65535 || temp_val < 0) {
                fprintf(stderr, "[ERR] %s must be a positive value between 0-65535", argv[i]);
                return -1;
            }

            LW_PORT = temp_val;
        } else if (match_option(argv[i], "-dev", "--developer"))  {
            LW_DEV_MODE = 1;
        } else if (match_option(argv[i], "-ck", "--certificate-key")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[ERR] %s requires a value\n", argv[i]);
                return -1;
            }
            LW_CERT_FILE = argv[++i];
            LW_CERT = 1;
        } else if (match_option(argv[i], "-pk", "--private-key")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[ERR] %s requires a value\n", argv[i]);
                return -1;
            }
            LW_KEY_FILE = argv[++i];
            LW_KEY = 1;
        } else if (match_option(argv[i], "-c", "--compress")) {
            LW_COMPRESS = 1;
        }
    } 

    (LW_CERT == 1 && LW_KEY == 1) ? LW_SSL_ENABLED = 1 : LW_SSL_ENABLED == 0;
  
    if (LW_DEV_MODE) {
        start_live_reload_server(8181, "./public");
    } else if (LW_SSL_ENABLED == 1) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        init_openssl();
        ssl_ctx = create_ssl_ctx();
        configure_ssl_ctx(ssl_ctx, LW_CERT_FILE, LW_KEY_FILE);
    }

    return 0;
}

void print_help()
{
    printf("Lower Framework is a lightweight, modular web framework written in C that speeds up development with its flexibility and high performance.\n");
    printf("It allows you to customize and extend modules easily to fit your needs.\n");
    printf("With LowPM, integrating external libraries and managing modules becomes simple and efficient, making your projects faster and more maintainable.\n");
    printf("-p <port> : Allows you to select a custom port other than 8080\n");
    printf("-d : Development mode (if change files content refresh current page)\n");
    printf("-v : Allows you to open verbose mode to get more detailed information\n");
    printf("-ck : Allows you to add a certificate file for using HTTPS within TSL/SSL (You need -pk parameter too.)\n");
    printf("-pk : Allows you to add a private key file for using HTTPS within TSL/SSL (You need -ck parameter too.)\n");
    printf("\n\nTryCatch™ - @mal1kore1ss & @p0unter");
}

/* utils.c 23/07/2025 Review
 * Rewrote the parameter controller. */
