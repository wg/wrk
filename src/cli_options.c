#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cli_options.h"
#include "config.h"
#include "http_parser.h"
#include "wrk.h"
#include "units.h"
#include "script.h"

/**
 * @param proxy_url Proxy server URL in format '1.2.3.4:8000'.
 * @return 0 on success, -1 on error.
 */
static int
parse_proxy_url(const char* proxy_url, struct config* cfg)
{
        char url[256] = {0};
        strcpy(url, proxy_url);

        const char* addr = strtok(url, ":");
        if (addr == NULL) {
             fprintf(stderr, "Bad proxy URL format: %s. "
                 "Expected: '1.2.3.4:8080\n", proxy_url);
             return -1;
        }

        const char* port = strtok(NULL, ":");
        if (port == NULL) {
            fprintf(stderr, "Failed to parse proxy port from '%s'\n",
                 proxy_url);
            return -1;
        }

        strcpy(cfg->proxy_addr, addr);
        strcpy(cfg->proxy_port, port);

        return 0;
}

/**
 * @return 0 on success, -1 on error.
 */
static int
parse_proxy_user_pass(const char* proxy_user_pass, struct config* cfg)
{
        char user_pass[256] = {0};
        strcpy(user_pass, proxy_user_pass);

        const char* username = strtok(user_pass, ":");
        if (username == NULL) {
             fprintf(stderr, "Bad proxy user:password format: %s. "
                 "Expected: 'username:password\n", proxy_user_pass);
             return -1;
        }

        const char* password = strtok(NULL, ":");
        if (password == NULL) {
            fprintf(stderr, "Failed to parse proxy user password from '%s'\n",
                 proxy_user_pass);
            return -1;
        }

        strcpy(cfg->proxy_username, username);
        strcpy(cfg->proxy_user_password, password);

        return 0;
}

static struct option longopts[] = {
    { "connections", required_argument, NULL, 'c' },
    { "duration",    required_argument, NULL, 'd' },
    { "threads",     required_argument, NULL, 't' },
    { "script",      required_argument, NULL, 's' },
    { "header",      required_argument, NULL, 'H' },
    { "latency",     no_argument,       NULL, 'L' },
    { "timeout",     required_argument, NULL, 'T' },
    { "proxy",       required_argument, NULL, 'x' },
    { "proxy-user",  required_argument, NULL, 'U' },
    { "help",        no_argument,       NULL, 'h' },
    { "version",     no_argument,       NULL, 'v' },
    { NULL,          0,                 NULL,  0  }
};

int parse_args(struct config *cfg, char **url, struct http_parser_url *parts, char **headers, int argc, char **argv) {
    char **header = headers;
    int c;
    char *proxy_url = NULL;
    char *proxy_user_pass = NULL;

    memset(cfg, 0, sizeof(struct config));
    cfg->threads     = 2;
    cfg->connections = 10;
    cfg->duration    = 10;
    cfg->timeout     = SOCKET_TIMEOUT_MS;

    while ((c = getopt_long(argc, argv, "t:c:d:s:H:T:x:U:Lrv?", longopts, NULL)) != -1) {
        switch (c) {
            case 't':
                if (scan_metric(optarg, &cfg->threads)) return -1;
                break;
            case 'c':
                if (scan_metric(optarg, &cfg->connections)) return -1;
                break;
            case 'd':
                if (scan_time(optarg, &cfg->duration)) return -1;
                break;
            case 's':
                cfg->script = optarg;
                break;
            case 'H':
                *header++ = optarg;
                break;
            case 'L':
                cfg->latency = true;
                break;
            case 'T':
                if (scan_time(optarg, &cfg->timeout)) return -1;
                cfg->timeout *= 1000;
                break;
            case 'v':
                printf("wrk %s [%s] ", VERSION, aeGetApiName());
                printf("Copyright (C) 2012 Will Glozer\n");
                break;
            case 'x':
                proxy_url = optarg;
                break;
            case 'U':
                proxy_user_pass = optarg;
                break;
            case 'h':
            case '?':
            case ':':
            default:
                return -1;
        }
    }

    if (optind == argc || !cfg->threads || !cfg->duration) return -1;

    if (!script_parse_url(argv[optind], parts)) {
        fprintf(stderr, "invalid URL: %s\n", argv[optind]);
        return -1;
    }

    if (!cfg->connections || cfg->connections < cfg->threads) {
        fprintf(stderr, "number of connections must be >= threads\n");
        return -1;
    }

    if (proxy_url != NULL) {
        if (parse_proxy_url(proxy_url, cfg) != 0) {
            return -1;
        }
    }

    if (proxy_user_pass != NULL) {
        if (parse_proxy_user_pass(proxy_user_pass, cfg) != 0) {
            return -1;
        }
    }

    *url    = argv[optind];
    *header = NULL;

    return 0;
}
