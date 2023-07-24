// Copyright (C) 2012 - Will Glozer.  All rights reserved.
#include "wrk.h"
#include "stats.h"
#include "types.h"

struct config wrk_cfg;
uint64_t wrk_complete = 0;
uint64_t wrk_bytes = 0;
uint64_t wrk_runtime_us = 0;
struct errors wrk_errors = {0};
struct statistics_t wrk_statistics;
char *wrk_request = NULL;

static struct sock sock = {.connect = sock_connect,
                           .close = sock_close,
                           .read = sock_read,
                           .write = sock_write,
                           .readable = sock_readable};

static struct http_parser_settings parser_settings = {.on_message_complete =
                                                          response_complete};

static volatile sig_atomic_t stop = 0;

static void handler(int sig) { stop = 1; }

int benchmark(char *url) {
  struct http_parser_url parts = {};
  if (!parse_url(url, &parts)) {
    fprintf(stderr, "invalid URL: %s\n", url);
    return 1;
  }

  char *schema = copy_url_part(url, &parts, UF_SCHEMA);
  char *host = copy_url_part(url, &parts, UF_HOST);
  char *port = copy_url_part(url, &parts, UF_PORT);
  char *service = port ? port : schema;

  if (!strncmp("https", schema, 5)) {
    if ((wrk_cfg.ctx = ssl_init()) == NULL) {
      fprintf(stderr, "unable to initialize SSL\n");
      ERR_print_errors_fp(stderr);
      return 1;
    }
    sock.connect = ssl_connect;
    sock.close = ssl_close;
    sock.read = ssl_read;
    sock.write = ssl_write;
    sock.readable = ssl_readable;
  }

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  wrk_statistics.latency = stats_alloc(wrk_cfg.timeout * 1000);
  wrk_statistics.requests = stats_alloc(MAX_THREAD_RATE_S);
  wrk_statistics.ttfb = stats_alloc(MAX_THREAD_RATE_S);
  thread *threads = zcalloc(wrk_cfg.threads * sizeof(thread));

  struct addrinfo *addr = NULL;
  lookup_service(host, service, &addr);

  if (addr == NULL) {
    char *msg = strerror(errno);
    fprintf(stderr, "unable to connect to %s:%s %s\n", host, service, msg);
    return 1;
  }

  wrk_cfg.host = host;

  for (uint64_t i = 0; i < wrk_cfg.threads; i++) {
    thread *t = &threads[i];
    t->loop = aeCreateEventLoop(10 + wrk_cfg.connections * 3);
    t->connections = wrk_cfg.connections / wrk_cfg.threads;
    t->addr = addr;

    if (!t->loop || pthread_create(&t->thread, NULL, &thread_main, t)) {
      char *msg = strerror(errno);
      fprintf(stderr, "unable to create thread %" PRIu64 ": %s\n", i, msg);
      return 2;
    }
  }

  struct sigaction sa = {
      .sa_handler = handler,
      .sa_flags = 0,
  };
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  uint64_t start = time_us();

  sleep(wrk_cfg.duration);
  stop = 1;

  for (uint64_t i = 0; i < wrk_cfg.threads; i++) {
    thread *t = &threads[i];
    pthread_join(t->thread, NULL);

    wrk_complete += t->complete;
    wrk_bytes += t->bytes;

    wrk_errors.connect += t->errors.connect;
    wrk_errors.read += t->errors.read;
    wrk_errors.write += t->errors.write;
    wrk_errors.timeout += t->errors.timeout;
    wrk_errors.status += t->errors.status;
  }

  wrk_runtime_us = time_us() - start;
  if (wrk_complete / wrk_cfg.connections > 0) {
    int64_t interval = wrk_runtime_us / (wrk_complete / wrk_cfg.connections);
    stats_correct(wrk_statistics.latency, interval);
  }

  return 0;
}

void *thread_main(void *arg) {
  thread *thread = arg;

  size_t length = 0;

  length = strlen(wrk_request);

  thread->cs = zcalloc(thread->connections * sizeof(connection));
  connection *c = thread->cs;

  for (uint64_t i = 0; i < thread->connections; i++, c++) {
    c->thread = thread;
    c->ssl = wrk_cfg.ctx ? SSL_new(wrk_cfg.ctx) : NULL;
    c->request = wrk_request;
    c->length = length;
    connect_socket(thread, c);
  }

  aeEventLoop *loop = thread->loop;
  aeCreateTimeEvent(loop, RECORD_INTERVAL_MS, record_rate, thread, NULL);

  thread->start = time_us();
  aeMain(loop);

  aeDeleteEventLoop(loop);
  zfree(thread->cs);

  return NULL;
}

static int connect_socket(thread *thread, connection *c) {
  struct addrinfo *addr = thread->addr;
  struct aeEventLoop *loop = thread->loop;
  int fd, flags;

  fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

  flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  if (connect(fd, addr->ai_addr, addr->ai_addrlen) == -1) {
    if (errno != EINPROGRESS)
      goto error;
  }

  flags = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));

  flags = AE_READABLE | AE_WRITABLE;
  if (aeCreateFileEvent(loop, fd, flags, socket_connected, c) == AE_OK) {
    c->parser.data = c;
    c->fd = fd;
    return fd;
  }

error:
  thread->errors.connect++;
  close(fd);
  return -1;
}

static int reconnect_socket(thread *thread, connection *c) {
  aeDeleteFileEvent(thread->loop, c->fd, AE_WRITABLE | AE_READABLE);
  sock.close(c);
  close(c->fd);
  return connect_socket(thread, c);
}

static int record_rate(aeEventLoop *loop, long long id, void *data) {
  thread *thread = data;

  if (thread->requests > 0) {
    uint64_t elapsed_ms = (time_us() - thread->start) / 1000;
    uint64_t requests = (thread->requests / (double)elapsed_ms) * 1000; // req/s

    stats_record(wrk_statistics.requests, requests);

    thread->requests = 0;
    thread->start = time_us();
  }

  if (stop)
    aeStop(loop);

  return RECORD_INTERVAL_MS;
}

static int delay_request(aeEventLoop *loop, long long id, void *data) {
  connection *c = data;
  c->delayed = false;  // TODO: need review
  aeCreateFileEvent(loop, c->fd, AE_WRITABLE, socket_writeable, c);
  return AE_NOMORE;
}

static int response_complete(http_parser *parser) {
  connection *c = parser->data;
  thread *thread = c->thread;
  uint64_t now = time_us();
  int status = parser->status_code;

  thread->complete++;
  thread->requests++;

  if (status > 399) {
    thread->errors.status++;
  }

  if (c->headers.buffer) {
    *c->headers.cursor++ = '\0';
    c->state = FIELD;
  }

  if (--c->pending == 0) {
    if (!stats_record(wrk_statistics.latency, now - c->start)) {
      thread->errors.timeout++;
    }
    aeCreateFileEvent(thread->loop, c->fd, AE_WRITABLE, socket_writeable, c);
  }

  if (!http_should_keep_alive(parser)) {
    reconnect_socket(thread, c);
    goto done;
  }

  http_parser_init(parser, HTTP_RESPONSE);

done:
  return 0;
}

static void socket_connected(aeEventLoop *loop, int fd, void *data, int mask) {
  connection *c = data;

  switch (sock.connect(c, wrk_cfg.host)) {
  case OK:
    break;
  case ERROR:
    goto error;
  case RETRY:
    return;
  }

  http_parser_init(&c->parser, HTTP_RESPONSE);
  c->written = 0;

  aeCreateFileEvent(c->thread->loop, fd, AE_READABLE, socket_readable, c);
  aeCreateFileEvent(c->thread->loop, fd, AE_WRITABLE, socket_writeable, c);

  return;

error:
  c->thread->errors.connect++;
  reconnect_socket(c->thread, c);
}

static void socket_writeable(aeEventLoop *loop, int fd, void *data, int mask) {
  connection *c = data;
  thread *thread = c->thread;

  if (c->delayed) {
    uint64_t delay = 0; // @TODO: need review
    aeDeleteFileEvent(loop, fd, AE_WRITABLE);
    aeCreateTimeEvent(loop, delay, delay_request, c, NULL);
    return;
  }

  if (!c->written) {
    c->start = time_us();
    c->pending = 1;
  }

  char *buf = c->request + c->written;
  size_t len = c->length - c->written;
  size_t n;

  switch (sock.write(c, buf, len, &n)) {
  case OK:
    break;
  case ERROR:
    goto error;
  case RETRY:
    return;
  }

  c->written += n;
  if (c->written == c->length) {
    c->written = 0;
    aeDeleteFileEvent(loop, fd, AE_WRITABLE);
  }

  return;

error:
  thread->errors.write++;
  reconnect_socket(thread, c);
}

static void socket_readable(aeEventLoop *loop, int fd, void *data, int mask) {
  connection *c = data;

  // Time to first byte
  if (c->body.length == 0) {
    stats_record(wrk_statistics.ttfb, time_us() - c->start);
  }

  size_t n;

  do {
    switch (sock.read(c, &n)) {
    case OK:
      break;
    case ERROR:
      goto error;
    case RETRY:
      return;
    }

    if (http_parser_execute(&c->parser, &parser_settings, c->buf, n) != n)
      goto error;
    if (n == 0 && !http_body_is_final(&c->parser))
      goto error;

    c->thread->bytes += n;
  } while (n == RECVBUF && sock.readable(c) > 0);

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

static char *copy_url_part(char *url, struct http_parser_url *parts,
                           enum http_parser_url_fields field) {
  char *part = NULL;

  if (parts->field_set & (1 << field)) {
    uint16_t off = parts->field_data[field].off;
    uint16_t len = parts->field_data[field].len;
    part = zcalloc(len + 1 * sizeof(char));
    memcpy(part, &url[off], len);
  }

  return part;
}

static int lookup_service(char *host, char *service, struct addrinfo **result) {
  struct addrinfo *addrs;
  struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
  int rc;

  if ((rc = getaddrinfo(host, service, &hints, &addrs)) != 0) {
    const char *msg = gai_strerror(rc);
    fprintf(stderr, "unable to resolve %s:%s %s\n", host, service, msg);
    return 1;
  }

  for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
    int fd, connected = 0;
    if ((fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) !=
        -1) {
      connected = connect(fd, addr->ai_addr, addr->ai_addrlen) == 0;
      close(fd);
    }
    if (connected) {
      *result = addr;
      break;
    }
  }
  // freeaddrinfo(addrs);
  return 0;
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
