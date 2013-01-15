#ifndef URLS_H
#define URLS_H

#ifndef URLS_MAX
#define URLS_MAX 100 * 1024 * 1024
#endif

#ifndef URLS_INC_STEP
#define URLS_INC_STEP 1024
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
