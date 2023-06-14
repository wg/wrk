#include <getopt.h>
#include "wrk.h"

void usage();
int parse_args(struct config *, char **, struct http_parser_url *, char **, int,
               char **);
void print_stats_header();
void print_stats(char *, stats *, char *(*)(long double));
void print_stats_latency(stats *);

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

  if (parse_args(&cfg, &url, &parts, headers, argc, argv)) {
    usage();
    exit(1);
  }

  char *time = format_time_s(cfg.duration);
  printf("Running %s test @ %s\n", time, url);
  printf("  %" PRIu64 " threads and %" PRIu64 " connections\n", cfg.threads,
         cfg.connections);

  wrk_run(url, headers, parts);

  long double runtime_s = runtime_us / 1000000.0;
  long double req_per_s = complete / runtime_s;
  long double bytes_per_s = bytes / runtime_s;

  if (complete / cfg.connections > 0) {
    int64_t interval = runtime_us / (complete / cfg.connections);
    stats_correct(statistics.latency, interval);
  }

  print_stats_header();
  print_stats("Latency", statistics.latency, format_time_us);
  print_stats("Req/Sec", statistics.requests, format_metric);
  print_stats("TTFB", statistics.ttfb, format_time_us);
  print_stats_latency(statistics.latency);

  char *runtime_msg = format_time_us(runtime_us);

  printf("  %" PRIu64 " requests in %s, %sB read\n", complete, runtime_msg,
         format_binary(bytes));
  if (errors.connect || errors.read || errors.write || errors.timeout) {
    printf("  Socket errors: connect %d, read %d, write %d, timeout %d\n",
           errors.connect, errors.read, errors.write, errors.timeout);
  }

  if (errors.status) {
    printf("  Non-2xx or 3xx responses: %d\n", errors.status);
  }

  printf("Requests/sec: %9.2Lf\n", req_per_s);
  printf("Transfer/sec: %10sB\n", format_binary(bytes_per_s));

  return 0;
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

void print_stats_header() {
  printf("  Thread Stats%6s%11s%8s%12s\n", "Avg", "Stdev", "Max", "+/- Stdev");
}

void print_units(long double n, char *(*fmt)(long double), int width) {
  char *msg = fmt(n);
  int len = strlen(msg), pad = 2;

  if (isalpha(msg[len - 1]))
    pad--;
  if (isalpha(msg[len - 2]))
    pad--;
  width -= pad;

  printf("%*.*s%.*s", width, width, msg, pad, "  ");

  free(msg);
}

void print_stats(char *name, stats *stats, char *(*fmt)(long double)) {
  uint64_t max = stats->max;
  long double mean = stats_mean(stats);
  long double stdev = stats_stdev(stats, mean);

  printf("    %-10s", name);
  print_units(mean, fmt, 8);
  print_units(stdev, fmt, 10);
  print_units(max, fmt, 9);
  printf("%8.2Lf%%\n", stats_within_stdev(stats, mean, stdev, 1));
}

void print_stats_latency(stats *stats) {
  long double percentiles[] = {50.0, 75.0, 90.0, 99.0};
  printf("  Latency Distribution\n");
  for (size_t i = 0; i < sizeof(percentiles) / sizeof(long double); i++) {
    long double p = percentiles[i];
    uint64_t n = stats_percentile(stats, p);
    printf("%7.0Lf%%", p);
    print_units(n, format_time_us, 10);
    printf("\n");
  }
}
