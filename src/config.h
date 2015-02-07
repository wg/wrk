#ifndef CONFIG_H
#define CONFIG_H

#if defined(__FreeBSD__) || defined(__APPLE__)
#define HAVE_KQUEUE
#elif defined(__linux__)
#define HAVE_EPOLL
#elif defined (__sun)
#define HAVE_EVPORT
#endif

#endif /* CONFIG_H */
