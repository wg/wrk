#ifndef WRK_H
#define WRK_H

#include "config.h"
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "stats.h"
#include "ae.h"
#include "http_parser.h"

#define VERSION  "2.2.0"
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
    SSL *ssl;
    uint64_t start;
    size_t written;
    char buf[RECVBUF];
} connection;

#endif /* WRK_H */
