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
#include "types.h"
#include "units.h"
#include "zmalloc.h"

extern struct config cfg;
extern struct statistics_t statistics;
extern uint64_t complete;
extern uint64_t bytes;
extern uint64_t runtime_us;
extern struct errors errors;
extern char* request;

int benchmark(char *);
int parse_url(char*, struct http_parser_url*);

static void *thread_main(void *);
static int connect_socket(thread *, connection *);
static int reconnect_socket(thread *, connection *);
static int lookup_service(char *, char *, struct addrinfo **);

static int record_rate(aeEventLoop *, long long, void *);

static void socket_connected(aeEventLoop *, int, void *, int);
static void socket_writeable(aeEventLoop *, int, void *, int);
static void socket_readable(aeEventLoop *, int, void *, int);

static int response_complete(http_parser *);

static uint64_t time_us();

static char *copy_url_part(char *, struct http_parser_url *,
                           enum http_parser_url_fields);

#endif /* WRK_H */
