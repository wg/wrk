#ifndef CONFIG_H
#define CONFIG_H

#if defined(__FreeBSD__) || defined(__APPLE__)
#define HAVE_KQUEUE
#elif defined(__linux__)
#define HAVE_EPOLL
#elif defined (__sun)
#define HAVE_EVPORT
#define _XPG6
#define __EXTENSIONS__
#include <stropts.h>
#include <sys/filio.h>
#include <sys/time.h>
#endif

#include <stdint.h>
#include <stdbool.h>

#include <openssl/ssl.h>


struct config {
    uint64_t connections;
    uint64_t duration;
    uint64_t threads;
    uint64_t timeout;
    uint64_t pipeline;
    bool     delay;
    bool     dynamic;
    bool     latency;
    char    *script;
    char     proxy_addr[256];
    char     proxy_port[16];
    char     proxy_username[256];
    char     proxy_user_password[256];
    SSL_CTX *ctx;
};

#endif /* CONFIG_H */
