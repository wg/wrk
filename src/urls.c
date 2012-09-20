#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <math.h>

#include "urls.h"
#include "aprintf.h"
#include "zmalloc.h"


static struct {
    uint64_t count;
    size_t size;
    url_request **requests;
} urls2;


static char *format_request(char *host, char *port, char *path, char **headers) {
    char *req = NULL;
    uint8_t has_host = 0;

    aprintf(&req, "GET %s HTTP/1.1\r\n", path);

    for (char **h = headers; *h != NULL; h++) {
        if (strncasecmp(*h, "Host:", strlen("Host:")) == 0)
            has_host = 1;
        aprintf(&req, "%s\r\n", *h);
    }

    if (!has_host) {
        aprintf(&req, "Host: %s", host);
        if (port) aprintf(&req, ":%s", port);
        aprintf(&req, "\r\n");
    }

    aprintf(&req, "\r\n");
    return req;
}

uint64_t urls_count() {
    return urls2.count;
}

int urls_add(char *host, char *port, char *path, char **headers) {
    url_request *req;
    if (urls2.count == urls2.size) {
        size_t new_size = urls2.size + URLS_INC_STEP;
        if (new_size > URLS_MAX) {
            fprintf(stderr, "Too many urls (max %i)\n", URLS_MAX);
            return -2;
        }
        urls2.requests = zrealloc(urls2.requests,
                                  new_size * sizeof(url_request *));
        urls2.size = new_size;
    }

    req = zcalloc(sizeof(url_request));
    if (req == NULL) return -1;

    req->buf = format_request(host, port, path, headers);
    req->size = req->buf ? strlen(req->buf) : 0;
    if (req->size == 0) {
        return -1;
    }

    urls2.requests[urls2.count] = req;
    urls2.count++;

    return 0;
}

url_request *urls_request(tinymt64_t *rand) {
    switch (urls2.count) {
        case 0: return NULL;
        case 1: return urls2.requests[0];
    }

    uint64_t n = tinymt64_generate_uint64(rand);
    return urls2.requests[n % urls2.count];
}

