#include "run.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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

static int safe_strtoi(const char *str, int min, int max)
{
    char *endptr;
    long value;

    if (!str)
        return -1;

    errno = 0;
    value = strtol(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0')
    {
        return -1;
    }

    if (value < min || value > max)
    {
        return -1;
    }

    return (int)value;
}

int parameter_controller(int argc, char *argv[])
{
    LW_PORT = 8080;
    LW_VERBOSE = 0;
    LW_DEV_MODE = 0;

    int opt;
    int temp_val;
    int option_index = 0;

    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"cert", required_argument, 0, 'c'},
        {"key", required_argument, 0, 'k'},
        {"help", no_argument, 0, '?'},
        {"verbose", no_argument, 0, 'v'},
        {"dev", no_argument, 0, 'd'},
        {0, 0, 0, 0}};

    optind = 1;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "p:c:k:vd", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (!optarg)
            {
                fprintf(stderr, "[ERR] Port option requires a value\n");
                fprintf(stderr, "[ERR] Usage: %s [-p port].\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
                exit(EXIT_FAILURE);
            }

            temp_val = safe_strtoi(optarg, 1, 65535);
            if (temp_val == -1)
            {
                fprintf(stderr, "[ERR] Invalid port: '%s' (must be 1-65535)\n", optarg);
                fprintf(stderr, "[ERR] Usage: %s [-p port]\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
                exit(EXIT_FAILURE);
            }

            LW_PORT = temp_val;
            break;
        case 'v':
            LW_VERBOSE = 1;
            break;
        case '?':
            if (optopt == 'p')
            {
                fprintf(stderr, "[INFO] -p <port>\n");
                fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
                exit(EXIT_FAILURE);
            } else if (optopt == 'c') {
                fprintf(stderr, "[INFO] -c <certificate_file>\n");
                fprintf(stderr, "[INFO] Example: %s [-c server.crt] or [-cserver.crt]", argv[0]);
                exit(EXIT_FAILURE);
            } else {
                print_help();
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            LW_DEV_MODE = 1;
            printf("[LW] Development Mode enabled (Hot Refresh)\n");
            break;
        case 'c':
            if (!optarg)
            {
                fprintf(stderr, "[ERR] Certificate option requires a value\n");
                fprintf(stderr, "[ERR] Usage: %s [-c certificate_file]\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-c server.crt] or [-cserver.crt]\n", argv[0]);
                exit(EXIT_FAILURE);
            } else if (optarg == NULL) {
                fprintf(stderr, "[ERR] Invalid usage: '%s'\n", optarg);
                fprintf(stderr, "[ERR] Usage: %s [-c certificate_file]\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-c server.crt] or [-cserver.crt]\n", argv[0]); 
                exit(EXIT_FAILURE);
            }

            LW_CERT_FILE = optarg;
            LW_CERT = 1;
            break;
        case 'k':
            if (!optarg)
            {
                fprintf(stderr, "[ERR] Private key option requires a value\n");
                fprintf(stderr, "[ERR] Usage: %s [-k key_file]\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-k server.key] or [-cserver.key]\n", argv[0]);
                exit(EXIT_FAILURE);
            } else if (optarg == NULL) {
                fprintf(stderr, "[ERR] Invalid usage: '%s'\n", optarg);
                fprintf(stderr, "[ERR] Usage: %s [-k key_file]\n", argv[0]);
                fprintf(stderr, "[INFO] Example: %s [-k server.key] or [-kserver.key]\n", argv[0]); 
                exit(EXIT_FAILURE);
            }

            LW_KEY_FILE = optarg;
            LW_KEY = 1;
            break;
        default:
            fprintf(stderr, "[ERR] Usage: %s [-p port] [-v] [-d]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    (LW_CERT == 1 && LW_KEY == 1) ? LW_SSL_ENABLED = 1 : LW_SSL_ENABLED == 0;

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
    printf("-c : Allows you to add a certificate file for using HTTPS within TSL/SSL (You need -k parameter too.)\n");
    printf("-k : Allows you to add a private key file for using HTTPS within TSL/SSL (You need -c parameter too.)\n");
    printf("\n\nTryCatch™ - @mal1kore1ss & @p0unter");
}
