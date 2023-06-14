// Copyright (C) 2012 - Will Glozer.  All rights reserved.
#include "wrk.h"
#include "stats.h"
#include "types.h"

struct config cfg;
uint64_t complete = 0;
uint64_t bytes = 0;
uint64_t runtime_us = 0;
struct errors errors = {0};
struct statistics_t statistics;

static struct sock sock = {.connect = sock_connect,
                           .close = sock_close,
                           .read = sock_read,
                           .write = sock_write,
                           .readable = sock_readable};

static struct http_parser_settings parser_settings = {.on_message_complete =
                                                          response_complete};

static volatile sig_atomic_t stop = 0;

static void handler(int sig) { stop = 1; }

void wrk_run(char *url, char **headers, struct http_parser_url parts) {
  char *schema = copy_url_part(url, &parts, UF_SCHEMA);
  char *host = copy_url_part(url, &parts, UF_HOST);
  char *port = copy_url_part(url, &parts, UF_PORT);
  char *service = port ? port : schema;

  if (!strncmp("https", schema, 5)) {
    if ((cfg.ctx = ssl_init()) == NULL) {
      fprintf(stderr, "unable to initialize SSL\n");
      ERR_print_errors_fp(stderr);
      exit(1);
    }
    sock.connect = ssl_connect;
    sock.close = ssl_close;
    sock.read = ssl_read;
    sock.write = ssl_write;
    sock.readable = ssl_readable;
  }

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  statistics.latency = stats_alloc(cfg.timeout * 1000);
  statistics.requests = stats_alloc(MAX_THREAD_RATE_S);
  statistics.ttfb = stats_alloc(MAX_THREAD_RATE_S);
  thread *threads = zcalloc(cfg.threads * sizeof(thread));

  struct addrinfo *addr = NULL;
  lookup_service(host, service, &addr);

  if (addr == NULL) {
    char *msg = strerror(errno);
    fprintf(stderr, "unable to connect to %s:%s %s\n", host, service, msg);
    exit(1);
  }

  cfg.pipeline = 1; // use when you want to send multiple other requests
  cfg.host = host;

  for (uint64_t i = 0; i < cfg.threads; i++) {
    thread *t = &threads[i];
    t->loop = aeCreateEventLoop(10 + cfg.connections * 3);
    t->connections = cfg.connections / cfg.threads;
    t->addr = addr;

    if (!t->loop || pthread_create(&t->thread, NULL, &thread_main, t)) {
      char *msg = strerror(errno);
      fprintf(stderr, "unable to create thread %" PRIu64 ": %s\n", i, msg);
      exit(2);
    }
  }

  struct sigaction sa = {
      .sa_handler = handler,
      .sa_flags = 0,
  };
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  uint64_t start = time_us();

  sleep(cfg.duration);
  stop = 1;

  for (uint64_t i = 0; i < cfg.threads; i++) {
    thread *t = &threads[i];
    pthread_join(t->thread, NULL);

    complete += t->complete;
    bytes += t->bytes;

    errors.connect += t->errors.connect;
    errors.read += t->errors.read;
    errors.write += t->errors.write;
    errors.timeout += t->errors.timeout;
    errors.status += t->errors.status;
  }

  runtime_us = time_us() - start;
  if (complete / cfg.connections > 0) {
    int64_t interval = runtime_us / (complete / cfg.connections);
    stats_correct(statistics.latency, interval);
  }
}

void *thread_main(void *arg) {
  thread *thread = arg;

  char *request = NULL;
  size_t length = 0;

  request = "GET / HTTP/1.1\nHost: localhost:8000\r\n\r\n";
  length = strlen(request);

  thread->cs = zcalloc(thread->connections * sizeof(connection));
  connection *c = thread->cs;

  for (uint64_t i = 0; i < thread->connections; i++, c++) {
    c->thread = thread;
    c->ssl = cfg.ctx ? SSL_new(cfg.ctx) : NULL;
    c->request = request;
    c->length = length;
    c->delayed = cfg.delay;
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

    stats_record(statistics.requests, requests);

    thread->requests = 0;
    thread->start = time_us();
  }

  if (stop)
    aeStop(loop);

  return RECORD_INTERVAL_MS;
}

static int delay_request(aeEventLoop *loop, long long id, void *data) {
  connection *c = data;
  c->delayed = false;
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
    if (!stats_record(statistics.latency, now - c->start)) {
      thread->errors.timeout++;
    }
    c->delayed = cfg.delay;
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

  switch (sock.connect(c, cfg.host)) {
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
    uint64_t delay = 0; // need review
    aeDeleteFileEvent(loop, fd, AE_WRITABLE);
    aeCreateTimeEvent(loop, delay, delay_request, c, NULL);
    return;
  }

  if (!c->written) {
    c->start = time_us();
    c->pending = cfg.pipeline;
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
    stats_record(statistics.ttfb, time_us() - c->start);
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

static void lookup_service(char *host, char *service,
                           struct addrinfo **result) {
  struct addrinfo *addrs;
  struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
  int rc;

  if ((rc = getaddrinfo(host, service, &hints, &addrs)) != 0) {
    const char *msg = gai_strerror(rc);
    fprintf(stderr, "unable to resolve %s:%s %s\n", host, service, msg);
    exit(1);
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
}
