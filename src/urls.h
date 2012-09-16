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

typedef struct {
	uint64_t count;
	url_request **requests;
} urls;

urls *urls_alloc(void);
void urls_free(urls *);
uint64_t urls_count(urls *);
int urls_add(urls *, char *host, char *port, char *path, char **headers);
url_request *urls_request(urls *, tinymt64_t *);

#endif /* URLS_H */
