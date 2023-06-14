#include "wrk.h"
#include <getopt.h>

void usage();
int parse_args(struct config *, char **, struct http_parser_url *, char **, int,
               char **);

struct option longopts[] = {{"connections", required_argument, NULL, 'c'},
                            {"duration", required_argument, NULL, 'd'},
                            {"threads", required_argument, NULL, 't'},
                            {"script", required_argument, NULL, 's'},
                            {"header", required_argument, NULL, 'H'},
                            {"latency", no_argument, NULL, 'L'},
                            {"timeout", required_argument, NULL, 'T'},
                            {"help", no_argument, NULL, 'h'},
                            {"version", no_argument, NULL, 'v'},
                            {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  char *url, **headers = zmalloc(argc * sizeof(char *));
  struct http_parser_url parts = {};
  struct config conf;

  if (parse_args(&conf, &url, &parts, headers, argc, argv)) {
    usage();
    exit(1);
  }

  return wrk_run(url, headers, conf, parts);
}

void usage() {
  printf("Usage: wrk <options> <url>                            \n"
         "  Options:                                            \n"
         "    -c, --connections <N>  Connections to keep open   \n"
         "    -d, --duration    <T>  Duration of test           \n"
         "    -t, --threads     <N>  Number of threads to use   \n"
         "                                                      \n"
         "    -s, --script      <S>  Load Lua script file       \n"
         "    -H, --header      <H>  Add header to request      \n"
         "        --latency          Print latency statistics   \n"
         "        --timeout     <T>  Socket/request timeout     \n"
         "    -v, --version          Print version details      \n"
         "                                                      \n"
         "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
         "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

int parse_url(char *url, struct http_parser_url *parts) {
  if (!http_parser_parse_url(url, strlen(url), 0, parts)) {
    if (!(parts->field_set & (1 << UF_SCHEMA)))
      return 0;
    if (!(parts->field_set & (1 << UF_HOST)))
      return 0;
    return 1;
  }
  return 0;
}

int parse_args(struct config *cfg, char **url, struct http_parser_url *parts,
               char **headers, int argc, char **argv) {
  char **header = headers;
  int c;

  memset(cfg, 0, sizeof(struct config));
  cfg->threads = 2;
  cfg->connections = 10;
  cfg->duration = 10;
  cfg->timeout = SOCKET_TIMEOUT_MS;

  while ((c = getopt_long(argc, argv, "t:c:d:s:H:T:Lrv?", longopts, NULL)) !=
         -1) {
    switch (c) {
    case 't':
      if (scan_metric(optarg, &cfg->threads))
        return -1;
      break;
    case 'c':
      if (scan_metric(optarg, &cfg->connections))
        return -1;
      break;
    case 'd':
      if (scan_time(optarg, &cfg->duration))
        return -1;
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
      if (scan_time(optarg, &cfg->timeout))
        return -1;
      cfg->timeout *= 1000;
      break;
    case 'v':
      printf("wrk %s [%s] ", VERSION, aeGetApiName());
      printf("Copyright (C) 2012 Will Glozer\n");
      break;
    case 'h':
    case '?':
    case ':':
    default:
      return -1;
    }
  }

  if (optind == argc || !cfg->threads || !cfg->duration)
    return -1;

  if (!parse_url(argv[optind], parts)) {
    fprintf(stderr, "invalid URL: %s\n", argv[optind]);
    return -1;
  }

  if (!cfg->connections || cfg->connections < cfg->threads) {
    fprintf(stderr, "number of connections must be >= threads\n");
    return -1;
  }

  *url = argv[optind];
  *header = NULL;

  return 0;
}
