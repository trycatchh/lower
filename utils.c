#include "run.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

int FW_PORT;
int FW_VERBOSE;

const char *method_to_string(http_method_t method) {
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case PUT: return "PUT";
        case DELETE: return "DELETE";
        case PATCH: return "PATCH";
        case HEAD: return "HEAD";
        case OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

route_t *find_route(http_method_t method, const char *path) {
    for (int i = 0; i < lw_ctx.route_count; i++) {
        if (lw_ctx.routes[i].method == method && 
            strcmp(lw_ctx.routes[i].path, path) == 0) {
            return &lw_ctx.routes[i];
        }
    }
    return NULL;
}

static int safe_strtoi(const char *str, int min, int max) {
    char *endptr;
    long value;
    
    if (!str) return -1;
    
    errno = 0;
    value = strtol(str, &endptr, 10);
    
    if (errno != 0 || *endptr != '\0') {
        return -1;
    }
    
    if (value < min || value > max) {
        return -1;
    }
    
    return (int)value;
}

int parameter_controller(int argc, char *argv[]) {
   
    FW_PORT = 8080;
    FW_VERBOSE = 0;

    int opt;
    int temp_val;
    int option_index = 0;

    static struct option long_options[] = {
	{"port", required_argument, 0, 'p'},
	{"help", no_argument, 0, '?'},
	{"verbose", no_argument, 0, 'v'},
	{0,0,0,0}
    };
    
    optind = 1;
    opterr = 0;
    
    while ((opt = getopt_long(argc, argv, "p:h:v", long_options, &option_index)) != -1) {
        switch(opt) {
            case 'p':
                if (!optarg) {
                    fprintf(stderr, "[ERR] Port option requires a value\n");
                    fprintf(stderr, "[ERR] Usage: %s [-p port].\n", argv[0]);
		    fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
		    exit(EXIT_FAILURE);
                }
                
                temp_val = safe_strtoi(optarg, 1, 65535);
                if (temp_val == -1) {
                    fprintf(stderr, "[ERR] Invalid port: '%s' (must be 1-65535)\n", optarg);
                    fprintf(stderr, "[ERR] Usage: %s [-p port]\n", argv[0]);
		    fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
		    exit(EXIT_FAILURE);
                }
                
                FW_PORT = temp_val;
                break; 
            case 'v':
                FW_VERBOSE = 1;
                break;
	    case 'h':
		break;
            case '?':
                if (optopt == 'p') {
                    fprintf(stderr, "[INFO] -p <port>\n");
		    fprintf(stderr, "[INFO] Example: %s [-p 8080] or [-p8080]\n", argv[0]);
		    exit(EXIT_FAILURE);
                } else {
                    print_help();
		    exit(EXIT_FAILURE);
		}
	    default:
                fprintf(stderr, "[ERR] Usage: %s [-p port] [-v]\n", argv[0]);
        	exit(EXIT_FAILURE);
	}
    }
     
    return 0;
}

void print_help() {
	printf("Lower Framework is a lightweight, modular web framework written in C that speeds up development with its flexibility and high performance.\n");
	printf("It allows you to customize and extend modules easily to fit your needs.\n");
	printf("With LowPM, integrating external libraries and managing modules becomes simple and efficient, making your projects faster and more maintainable.\n");
	printf("-p <port> : Allows you to select a custom port other than 8080\n");
	printf("-v : Allows you to open verbose mode to get more detailed information (Not finished yet!)\n");
	printf("\n\nTryCatch™ - @mal1kore1ss & @p0unter");
}
