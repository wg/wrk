#ifndef WRK_H
#define WRK_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "aprintf.h"
#include "ssl.h"
#include "stats.h"
#include "units.h"
#include "zmalloc.h"

struct config {
  uint64_t connections;
  uint64_t duration;
  uint64_t threads;
  uint64_t timeout;
  uint64_t pipeline;
  bool delay;
  bool dynamic;
  bool latency;
  char *host;
  char *script;
  SSL_CTX *ctx;
};

int wrk_run(char *, char **, struct config, struct http_parser_url);

static void *thread_main(void *);
static int connect_socket(thread *, connection *);
static int reconnect_socket(thread *, connection *);
static void lookup_service(char *, char *, struct addrinfo **);

static int record_rate(aeEventLoop *, long long, void *);

static void socket_connected(aeEventLoop *, int, void *, int);
static void socket_writeable(aeEventLoop *, int, void *, int);
static void socket_readable(aeEventLoop *, int, void *, int);

static int response_complete(http_parser *);

static uint64_t time_us();

static char *copy_url_part(char *, struct http_parser_url *,
                           enum http_parser_url_fields);

static void print_stats_header();
static void print_stats(char *, stats *, char *(*)(long double));
static void print_stats_latency(stats *);

#endif /* WRK_H */
