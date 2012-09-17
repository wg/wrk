#ifndef URLS_H
#define URLS_H

#ifndef URLS_MAX
#define URLS_MAX 10240
#endif

#include "tinymt64.h"

typedef struct {
    size_t size;
    char *buf;
} url_request;

uint64_t urls_count();
int urls_add(char *host, char *port, char *path, char **headers);
url_request *urls_request(tinymt64_t *);

#endif /* URLS_H */
