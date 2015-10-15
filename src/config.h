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

#endif /* CONFIG_H */
