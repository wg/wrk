// Copyright (C) 2012 - Will Glozer.  All rights reserved.

#include "wrk.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>

#include "aprintf.h"
#include "stats.h"
#include "units.h"
#include "zmalloc.h"
#include "tinymt64.h"

static struct config {
    struct addrinfo addr;
    uint64_t threads;
    uint64_t connections;
    uint64_t duration;
    uint64_t timeout;
} cfg;

static struct {
    size_t size;
    char *buf;
} request;

static struct {
    stats *latency;
    stats *requests;
    pthread_mutex_t mutex;
} statistics;

static const struct http_parser_settings parser_settings = {
    .on_message_complete = request_complete
};

static volatile sig_atomic_t stop = 0;

static void handler(int sig) {
    stop = 1;
}

static void usage() {
    printf("Usage: wrk <options> <url>                            \n"
           "  Options:                                            \n"
           "    -c, --connections <N>  Connections to keep open   \n"
           "    -d, --duration    <T>  Duration of test           \n"
           "    -t, --threads     <N>  Number of threads to use   \n"
           "                                                      \n"
           "    -H, --header      <H>  Add header to request      \n"
           "    -v, --version          Print version details      \n"
           "                                                      \n"
           "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
           "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

int main(int argc, char **argv) {
    struct addrinfo *addrs, *addr;
    struct http_parser_url parser_url;
    char *url, **headers;
    int rc;

    headers = zmalloc((argc / 2) * sizeof(char *));

    if (parse_args(&cfg, &url, headers, argc, argv)) {
        usage();
        exit(1);
    }

    if (http_parser_parse_url(url, strlen(url), 0, &parser_url)) {
        fprintf(stderr, "invalid URL: %s\n", url);
        exit(1);
    }

    char *host = extract_url_part(url, &parser_url, UF_HOST);
    char *port = extract_url_part(url, &parser_url, UF_PORT);
    char *service = port ? port : extract_url_part(url, &parser_url, UF_SCHEMA);
    char *path = "/";

    if (parser_url.field_set & (1 << UF_PATH)) {
        path = &url[parser_url.field_data[UF_PATH].off];
    }

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    if ((rc = getaddrinfo(host, service, &hints, &addrs)) != 0) {
        const char *msg = gai_strerror(rc);
        fprintf(stderr, "unable to resolve %s:%s %s\n", host, service, msg);
        exit(1);
    }

    for (addr = addrs; addr != NULL; addr = addr->ai_next) {
        int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd == -1) continue;
        rc = connect(fd, addr->ai_addr, addr->ai_addrlen);
        close(fd);
        if (rc == 0) break;
    }

    if (addr == NULL) {
        char *msg = strerror(errno);
        fprintf(stderr, "unable to connect to %s:%s %s\n", host, service, msg);
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    cfg.addr     = *addr;
    request.buf  = format_request(host, port, path, headers);
    request.size = strlen(request.buf);

    pthread_mutex_init(&statistics.mutex, NULL);
    statistics.latency  = stats_alloc(SAMPLES);
    statistics.requests = stats_alloc(SAMPLES);

    thread *threads = zcalloc(cfg.threads * sizeof(thread));
    uint64_t connections = cfg.connections / cfg.threads;
    uint64_t stop_at     = time_us() + (cfg.duration * 1000000);

    for (uint64_t i = 0; i < cfg.threads; i++) {
        thread *t = &threads[i];
        t->connections = connections;
        t->stop_at     = stop_at;

        if (pthread_create(&t->thread, NULL, &thread_main, t)) {
            char *msg = strerror(errno);
            fprintf(stderr, "unable to create thread %"PRIu64" %s\n", i, msg);
            exit(2);
        }
    }

    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags   = 0,
    };
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    char *time = format_time_s(cfg.duration);
    printf("Running %s test @ %s\n", time, url);
    printf("  %"PRIu64" threads and %"PRIu64" connections\n", cfg.threads, cfg.connections);

    uint64_t start    = time_us();
    uint64_t complete = 0;
    uint64_t bytes    = 0;
    errors errors     = { 0 };

    for (uint64_t i = 0; i < cfg.threads; i++) {
        thread *t = &threads[i];
        pthread_join(t->thread, NULL);

        complete += t->complete;
        bytes    += t->bytes;

        errors.connect += t->errors.connect;
        errors.read    += t->errors.read;
        errors.write   += t->errors.write;
        errors.timeout += t->errors.timeout;
        errors.status  += t->errors.status;
    }

    uint64_t runtime_us = time_us() - start;
    long double runtime_s   = runtime_us / 1000000.0;
    long double req_per_s   = complete   / runtime_s;
    long double bytes_per_s = bytes      / runtime_s;

    print_stats_header();
    print_stats("Latency", statistics.latency, format_time_us);
    print_stats("Req/Sec", statistics.requests, format_metric);

    char *runtime_msg = format_time_us(runtime_us);

    printf("  %"PRIu64" requests in %s, %sB read\n", complete, runtime_msg, format_binary(bytes));
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

void *thread_main(void *arg) {
    thread *thread = arg;

    aeEventLoop *loop = aeCreateEventLoop(10 + cfg.connections * 3);
    thread->cs   = zmalloc(thread->connections * sizeof(connection));
    thread->loop = loop;
    tinymt64_init(&thread->rand, time_us());

    connection *c = thread->cs;

    for (uint64_t i = 0; i < thread->connections; i++, c++) {
        c->thread  = thread;
        c->latency = 0;
        connect_socket(thread, c);
    }

    aeCreateTimeEvent(loop, SAMPLE_INTERVAL_MS, sample_rate, thread, NULL);
    aeCreateTimeEvent(loop, TIMEOUT_INTERVAL_MS, check_timeouts, thread, NULL);

    thread->start = time_us();
    aeMain(loop);

    aeDeleteEventLoop(loop);
    zfree(thread->cs);

    return NULL;
}

static int connect_socket(thread *thread, connection *c) {
    struct addrinfo addr = cfg.addr;
    struct aeEventLoop *loop = thread->loop;
    int fd, flags;

    fd = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    if (connect(fd, addr.ai_addr, addr.ai_addrlen) == -1) {
        if (errno != EINPROGRESS) {
            thread->errors.connect++;
            goto error;
        }
    }

    flags = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));

    if (aeCreateFileEvent(loop, fd, AE_WRITABLE, socket_writeable, c) != AE_OK) {
        goto error;
    }

    http_parser_init(&c->parser, HTTP_RESPONSE);
    c->parser.data = c;
    c->fd = fd;

    return fd;

  error:
    close(fd);
    return -1;
}

static int reconnect_socket(thread *thread, connection *c) {
    aeDeleteFileEvent(thread->loop, c->fd, AE_WRITABLE | AE_READABLE);
    close(c->fd);
    return connect_socket(thread, c);
}

static int sample_rate(aeEventLoop *loop, long long id, void *data) {
    thread *thread = data;

    uint64_t n = rand64(&thread->rand, thread->connections);
    uint64_t elapsed_ms = (time_us() - thread->start) / 1000;
    connection *c = thread->cs + n;
    uint64_t requests = (thread->complete / elapsed_ms) * 1000;

    pthread_mutex_lock(&statistics.mutex);
    stats_record(statistics.latency,  c->latency);
    stats_record(statistics.requests, requests);
    pthread_mutex_unlock(&statistics.mutex);

    return SAMPLE_INTERVAL_MS + rand64(&thread->rand, SAMPLE_INTERVAL_MS);
}

static int request_complete(http_parser *parser) {
    connection *c = parser->data;
    thread *thread = c->thread;
    uint64_t now = time_us();

    thread->complete++;
    c->latency = now - c->start;

    if (parser->status_code > 399) {
        thread->errors.status++;
    }

    if (now >= thread->stop_at) {
        aeStop(thread->loop);
        goto done;
    }

    if (!http_should_keep_alive(parser)) goto reconnect;

    http_parser_init(parser, HTTP_RESPONSE);
    aeDeleteFileEvent(thread->loop, c->fd, AE_READABLE);
    aeCreateFileEvent(thread->loop, c->fd, AE_WRITABLE, socket_writeable, c);

    goto done;

  reconnect:
    reconnect_socket(thread, c);

  done:
    return 0;
}

static int check_timeouts(aeEventLoop *loop, long long id, void *data) {
    thread *thread = data;
    connection *c  = thread->cs;
    uint64_t now   = time_us();

    uint64_t maxAge = now - (cfg.timeout * 1000);

    for (uint64_t i = 0; i < thread->connections; i++, c++) {
        if (maxAge > c->start) {
            thread->errors.timeout++;
        }
    }

    if (stop || now >= thread->stop_at) {
        aeStop(loop);
    }

    return TIMEOUT_INTERVAL_MS;
}

static void socket_writeable(aeEventLoop *loop, int fd, void *data, int mask) {
    connection *c = data;

    if (write(fd, request.buf, request.size) < request.size) goto error;
    c->start = time_us();
    aeDeleteFileEvent(loop, fd, AE_WRITABLE);
    aeCreateFileEvent(loop, fd, AE_READABLE, socket_readable, c);

    return;

  error:
    c->thread->errors.write++;
    reconnect_socket(c->thread, c);
}

static void socket_readable(aeEventLoop *loop, int fd, void *data, int mask) {
    connection *c = data;
    ssize_t n;

    if ((n = read(fd, c->buf, sizeof(c->buf))) <= 0) goto error;
    if (http_parser_execute(&c->parser, &parser_settings, c->buf, n) != n) goto error;
    c->thread->bytes += n;

    return;

  error:
    c->thread->errors.read++;
    reconnect_socket(c->thread, c);
}

static uint64_t time_us() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000) + t.tv_usec;
}

static uint64_t rand64(tinymt64_t *state, uint64_t n) {
    uint64_t x, max = ~UINT64_C(0);
    max -= max % n;
    do {
        x = tinymt64_generate_uint64(state);
    } while (x >= max);
    return x % n;
}

static char *extract_url_part(char *url, struct http_parser_url *parser_url, enum http_parser_url_fields field) {
    char *part = NULL;

    if (parser_url->field_set & (1 << field)) {
        uint16_t off = parser_url->field_data[field].off;
        uint16_t len = parser_url->field_data[field].len;
        part = zcalloc(len + 1 * sizeof(char));
        memcpy(part, &url[off], len);
    }

    return part;
}

static char *format_request(char *host, char *port, char *path, char **headers) {
    char *req  = NULL;
    char *head = NULL;

    for (char **h = headers; *h != NULL; h++) {
        aprintf(&head, "%s\r\n", *h);
        if (!strncasecmp(*h, "Host:", 5)) {
            host = NULL;
            port = NULL;
        }
    }

    aprintf(&req, "GET %s HTTP/1.1\r\n", path);
    if (host) aprintf(&req, "Host: %s", host);
    if (port) aprintf(&req, ":%s", port);
    if (host) aprintf(&req, "\r\n");
    aprintf(&req, "%s\r\n", head ? head : "");

    free(head);
    return req;
}

static struct option longopts[] = {
    { "connections", required_argument, NULL, 'c' },
    { "duration",    required_argument, NULL, 'd' },
    { "threads",     required_argument, NULL, 't' },
    { "header",      required_argument, NULL, 'H' },
    { "help",        no_argument,       NULL, 'h' },
    { "version",     no_argument,       NULL, 'v' },
    { NULL,          0,                 NULL,  0  }
};

static int parse_args(struct config *cfg, char **url, char **headers, int argc, char **argv) {
    char c, **header = headers;

    memset(cfg, 0, sizeof(struct config));
    cfg->threads     = 2;
    cfg->connections = 10;
    cfg->duration    = 10;
    cfg->timeout     = SOCKET_TIMEOUT_MS;

    while ((c = getopt_long(argc, argv, "t:c:d:H:rv?", longopts, NULL)) != -1) {
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
            case 'H':
                *header++ = optarg;
                break;
            case 'v':
                printf("wrk %s [%s] ", VERSION, aeGetApiName());
                printf("Copyright (C) 2012 Will Glozer\n");
                break;
            case 'r':
                fprintf(stderr, "wrk 2.0.0+ uses -d instead of -r\n");
            case 'h':
            case '?':
            case ':':
            default:
                return -1;
        }
    }

    if (optind == argc || !cfg->threads || !cfg->duration) return -1;

    if (!cfg->connections || cfg->connections < cfg->threads) {
        fprintf(stderr, "number of connections must be >= threads\n");
        return -1;
    }

    *url    = argv[optind];
    *header = NULL;

    return 0;
}

static void print_stats_header() {
    printf("  Thread Stats%6s%11s%8s%12s\n", "Avg", "Stdev", "Max", "+/- Stdev");
}

static void print_units(long double n, char *(*fmt)(long double), int width) {
    char *msg = fmt(n);
    int len = strlen(msg), pad = 2;

    if (isalpha(msg[len-1])) pad--;
    if (isalpha(msg[len-2])) pad--;
    width -= pad;

    printf("%*.*s%.*s", width, width, msg, pad, "  ");

    free(msg);
}

static void print_stats(char *name, stats *stats, char *(*fmt)(long double)) {
    long double mean  = stats_mean(stats);
    long double max   = stats_max(stats);
    long double stdev = stats_stdev(stats, mean);

    printf("    %-10s", name);
    print_units(mean,  fmt, 8);
    print_units(stdev, fmt, 10);
    print_units(max,   fmt, 9);
    printf("%8.2Lf%%\n", stats_within_stdev(stats, mean, stdev, 1));
}
