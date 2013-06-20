#ifndef WRK_H
#define WRK_H

#include "config.h"
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>

#include "stats.h"
#include "ae.h"
#include "http_parser.h"
#include "tinymt64.h"

#define VERSION  "2.1.0"
#define RECVBUF  8192
#define SAMPLES  100000000

#define SOCKET_TIMEOUT_MS   2000
#define SAMPLE_INTERVAL_MS  10
#define CALIBRATE_DELAY_MS  500
#define TIMEOUT_INTERVAL_MS 2000

typedef struct {
    uint32_t connect;
    uint32_t read;
    uint32_t write;
    uint32_t status;
    uint32_t timeout;
} errors;

typedef struct {
    pthread_t thread;
    aeEventLoop *loop;
    uint64_t connections;
    uint64_t stop_at;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t start;
    uint64_t rate;
    uint64_t missed;
    stats *latency;
    tinymt64_t rand;
    errors errors;
    struct connection *cs;
} thread;

typedef struct connection {
    thread *thread;
    http_parser parser;
    int fd;
    uint64_t start;
    uint64_t pending;
    // offset into req.size for the start of the unwritten bytes of this batch
    int write_offset;
    char buf[RECVBUF];
} connection;

struct config;

static void *thread_main(void *);
static int connect_socket(thread *, connection *);
static int reconnect_socket(thread *, connection *);

static int calibrate(aeEventLoop *, long long, void *);
static int sample_rate(aeEventLoop *, long long, void *);
static int check_timeouts(aeEventLoop *, long long, void *);

static int write_batch(connection *c);
static void socket_writeable(aeEventLoop *, int, void *, int);
static void socket_readable(aeEventLoop *, int, void *, int);
static int request_complete(http_parser *);

static uint64_t time_us();

static char *extract_url_part(char *, struct http_parser_url *, enum http_parser_url_fields);
static char *format_request(char *, char *, char *, char **);

static int parse_args(struct config *, char **, char **, int, char **);
static void print_stats_header();
static void print_stats(char *, stats *, char *(*)(long double));
static void print_stats_latency(stats *);

#endif /* WRK_H */
