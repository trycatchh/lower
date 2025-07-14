#include "run.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

int LW_PORT;
int LW_VERBOSE;
int LW_DEV_MODE = 0; // Dev mode flag

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
        {"help", no_argument, 0, '?'},
        {"verbose", no_argument, 0, 'v'},
        {"dev", no_argument, 0, 'd'},
        {0, 0, 0, 0}};

    optind = 1;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "p:h:vd", long_options, &option_index)) != -1)
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
        case 'h':
            break;
        case '?':
            if (optopt == 'p')
            {
                fprintf(stderr, "[INFO] -p <port>\n");
                fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            else
            {
                print_help();
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            LW_DEV_MODE = 1;
            // printf("[LW] Enable Development Mode (Hot Refresh)\n");
            // start_live_reload_server(LW_PORT + 1000, "./public");
            break;
        default:
            fprintf(stderr, "[ERR] Usage: %s [-p port] [-v] [-d]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
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
    printf("-v : Allows you to open verbose mode to get more detailed information (Not finished yet!)\n");
    printf("\n\nTryCatch™ - @mal1kore1ss & @p0unter");
}
