#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "urls.h"
#include "aprintf.h"
#include "zmalloc.h"

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

static void url_request_free(url_request *req) {
	if (req->buf) zfree(req->buf);
	zfree(req);
}

urls *urls_alloc() {
	urls *urls = zcalloc(sizeof(urls));
	urls->count = 0;
	return urls;
}

void urls_free(urls *urls) {
	for (uint64_t i = 0; i < urls->count; i++) {
		url_request_free(urls->requests[i]);
	}
	zfree(urls);
}

uint64_t urls_count(urls *urls) {
	return urls->count;
}

int urls_add(urls *urls, char *host, char *port, char *path, char **headers) {
	if (urls->count >= URLS_MAX) return -1;

	url_request *req = zcalloc(sizeof(url_request));
	if (req == NULL) return -1;

	req->buf = format_request(host, port, path, headers);
	req->size = req->buf ? strlen(req->buf) : 0;
	if (req->size == 0) {
		url_request_free(req);
		return -1;
	}

	urls->requests[urls->count] = req;
	urls->count++;

	return 0;
}

url_request *urls_request(urls *urls, tinymt64_t *rand) {
	switch (urls->count) {
		case 0: return NULL;
		case 1: return urls->requests[0];
	}

	uint64_t n = tinymt64_generate_uint64(rand);
	return urls->requests[n % urls->count];
}

