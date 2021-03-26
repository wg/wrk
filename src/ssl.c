// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <pthread.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ssl.h"

SSL_CTX *ssl_init(int ssl_proto_version, char *ssl_cipher) {
    SSL_CTX *ctx = NULL;

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    ctx = SSL_CTX_new(SSLv23_method());

    if (ctx == NULL) {
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_verify_depth(ctx, 0);
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);

    if (ssl_proto_version == 0) {
#ifdef SSL_CTX_set_min_proto_version
        SSL_CTX_set_min_proto_version(ctx, 0);
#ifdef TLS1_3_VERSION
        SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
#else
        SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);
#endif
#endif

    } else {
#ifdef SSL_CTX_set_min_proto_version
        SSL_CTX_set_min_proto_version(ctx, ssl_proto_version);
        SSL_CTX_set_max_proto_version(ctx, ssl_proto_version);
#endif
    }

    if (ssl_cipher != NULL) 
        fprintf(stderr, "SETTING CIPHER: %s\n", ssl_cipher);
        if (!SSL_CTX_set_cipher_list(ctx, ssl_cipher)) {
            fprintf(stderr, "error setting cipher list [%s]\n", ssl_cipher);
            ERR_print_errors_fp(stderr);
            exit(1);
        }

    return ctx;
}

status ssl_connect(connection *c, char *host) {
    int r;
    SSL_set_fd(c->ssl, c->fd);
    SSL_set_tlsext_host_name(c->ssl, host);
    if ((r = SSL_connect(c->ssl)) != 1) {
        switch (SSL_get_error(c->ssl, r)) {
            case SSL_ERROR_WANT_READ:  return RETRY;
            case SSL_ERROR_WANT_WRITE: return RETRY;
            default:                   return ERROR;
        }
    }
    return OK;
}

status ssl_close(connection *c) {
    SSL_shutdown(c->ssl);
    SSL_clear(c->ssl);
    return OK;
}

status ssl_read(connection *c, size_t *n) {
    int r;
    if ((r = SSL_read(c->ssl, c->buf, sizeof(c->buf))) <= 0) {
        switch (SSL_get_error(c->ssl, r)) {
            case SSL_ERROR_WANT_READ:  return RETRY;
            case SSL_ERROR_WANT_WRITE: return RETRY;
            default:                   return ERROR;
        }
    }
    *n = (size_t) r;
    return OK;
}

status ssl_write(connection *c, char *buf, size_t len, size_t *n) {
    int r;
    if ((r = SSL_write(c->ssl, buf, len)) <= 0) {
        switch (SSL_get_error(c->ssl, r)) {
            case SSL_ERROR_WANT_READ:  return RETRY;
            case SSL_ERROR_WANT_WRITE: return RETRY;
            default:                   return ERROR;
        }
    }
    *n = (size_t) r;
    return OK;
}

size_t ssl_readable(connection *c) {
    return SSL_pending(c->ssl);
}
