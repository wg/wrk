#ifndef __CONFIG_H
#define __CONFIG_H

#if defined(__FreeBSD__) || defined(__APPLE__)
#define HAVE_KQUEUE
#elif defined(__linux__)
#define HAVE_EPOLL
#define _POSIX_C_SOURCE 200809L
#endif

#endif /* __CONFIG_H */
