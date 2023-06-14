#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ae.h"
#include "http_parser.h"
#include "stats.h"

#define RECVBUF 8192

#define MAX_THREAD_RATE_S 10000000
#define SOCKET_TIMEOUT_MS 2000
#define RECORD_INTERVAL_MS 100

extern const char *VERSION;

typedef struct {
  pthread_t thread;
  aeEventLoop *loop;
  struct addrinfo *addr;
  uint64_t connections;
  uint64_t complete;
  uint64_t requests;
  uint64_t bytes;
  uint64_t start;
  struct errors errors;
  struct connection *cs;
} thread;

struct statistics_t {
  stats *latency;
  stats *requests;
  stats *ttfb;
};

typedef struct {
  char *buffer;
  size_t length;
  char *cursor;
} buffer;

typedef struct connection {
  thread *thread;
  http_parser parser;
  enum { FIELD, VALUE } state;
  int fd;
  SSL *ssl;
  bool delayed;
  uint64_t start;
  char *request;
  size_t length;
  size_t written;
  uint64_t pending;
  buffer headers;
  buffer body;
  char buf[RECVBUF];
} connection;

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

#endif /* TYPES_H */
