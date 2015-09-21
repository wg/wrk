#ifndef WRK_CLI_OPTIONS
#define WRK_CLI_OPTIONS

struct http_parser_url;
struct config;

/**
 * @param cfg config structure filled with parsed options.
 * @param url target URL.
 * @return 0 on success, -1 on error.
 */
int parse_args(struct config *cfg, char **url, struct http_parser_url *parts,
    char **headers, int argc, char **argv);

#endif /* WRK_CLI_OPTIONS */
