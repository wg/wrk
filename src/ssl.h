#ifndef SSL_H
#define SSL_H

#include "net.h"

SSL_CTX *ssl_init(char *, char *, char*, char*);

status ssl_connect(connection *, char *);
status ssl_close(connection *);
status ssl_read(connection *, size_t *);
status ssl_write(connection *, char *, size_t, size_t *);
size_t ssl_readable(connection *);

#endif /* SSL_H */
