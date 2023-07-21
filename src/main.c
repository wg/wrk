#include <getopt.h>
#include "wrk.h"

void usage();
int parse_args(struct config *, char **, char **, int, char **);
void print_stats_header();
void print_stats(char *, stats *, char *(*)(long double));
void print_stats_latency(stats *);

struct option longopts[] = {{"connections", required_argument, NULL, 'c'},
                            {"duration", required_argument, NULL, 'd'},
                            {"threads", required_argument, NULL, 't'},
                            {"header", required_argument, NULL, 'H'},
                            {"timeout", required_argument, NULL, 'T'},
                            {"help", no_argument, NULL, 'h'},
                            {"version", no_argument, NULL, 'v'},
                            {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  char *url, **headers = zmalloc(argc * sizeof(char *));

  if (parse_args(&wrk_cfg, &url, headers, argc, argv)) {
    usage();
    exit(1);
  }

  char *time = format_time_s(wrk_cfg.duration);
  printf("Running %s test @ %s\n", time, url);
  printf("  %" PRIu64 " threads and %" PRIu64 " connections\n", wrk_cfg.threads,
         wrk_cfg.connections);

  wrk_request = "GET / HTTP/1.1\nHost: localhost:8000\r\n\r\n";
  benchmark(url);

  long double runtime_s = wrk_runtime_us / 1000000.0;
  long double req_per_s = wrk_complete / runtime_s;
  long double bytes_per_s = wrk_bytes / runtime_s;

  if (wrk_complete / wrk_cfg.connections > 0) {
    int64_t interval = wrk_runtime_us / (wrk_complete / wrk_cfg.connections);
    stats_correct(wrk_statistics.latency, interval);
  }

  print_stats_header();
  print_stats("Latency", wrk_statistics.latency, format_time_us);
  print_stats("Req/Sec", wrk_statistics.requests, format_metric);
  print_stats("TTFB", wrk_statistics.ttfb, format_time_us);
  print_stats_latency(wrk_statistics.latency);

  char *runtime_msg = format_time_us(wrk_runtime_us);

  printf("  %" PRIu64 " requests in %s, %sB read\n", wrk_complete, runtime_msg,
         format_binary(wrk_bytes));
  if (wrk_errors.connect || wrk_errors.read || wrk_errors.write || wrk_errors.timeout) {
    printf("  Socket errors: connect %d, read %d, write %d, timeout %d\n",
           wrk_errors.connect, wrk_errors.read, wrk_errors.write, wrk_errors.timeout);
  }

  if (wrk_errors.status) {
    printf("  Non-2xx or 3xx responses: %d\n", wrk_errors.status);
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
         "    -H, --header      <H>  Add header to request      \n"
         "        --timeout     <T>  Socket/request timeout     \n"
         "    -v, --version          Print version details      \n"
         "                                                      \n"
         "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
         "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

int parse_args(struct config *cfg, char **url, char **headers, int argc, char **argv) {
  char **header = headers;
  int c;

  memset(cfg, 0, sizeof(struct config));
  cfg->threads = 2;
  cfg->connections = 10;
  cfg->duration = 10;
  cfg->timeout = SOCKET_TIMEOUT_MS;

  while ((c = getopt_long(argc, argv, "t:c:d:H:T:rv?", longopts, NULL)) !=
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
    case 'H':
      *header++ = optarg;
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
