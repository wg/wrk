// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <pthread.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ssl.h"

static pthread_mutex_t *locks;
int ssl_data_index;

static void ssl_lock(int mode, int n, const char *file, int line) {
    pthread_mutex_t *lock = &locks[n];
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(lock);
    } else {
        pthread_mutex_unlock(lock);
    }
}

static unsigned long ssl_id() {
    return (unsigned long) pthread_self();
}

int new_session_callback(SSL * ssl, SSL_SESSION * session){
    connection * c = SSL_get_ex_data(ssl,ssl_data_index);
    if(c && c->cache){
        if(c->cache->cached_session){
            SSL_SESSION_free(c->cache->cached_session);
            SSL_SESSION * session = c->cache->cached_session;
            printf("free %p %ld %ld %d\n",session,SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
            c->cache->cached_session=NULL;
        }
        c->cache->cached_session=session;
        //CRYPTO_add(&session->references, 1, CRYPTO_LOCK_SSL_SESSION);
        //SSL_SESSION_free(session);
        //printf("insert %p %ld %ld %d\n",session,SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
    }
    return 1;
}

void ssl_info_callback(const SSL * ssl, int where, int ret){

    /*
    if(where & SSL_CB_HANDSHAKE_START){
        {
            connection * c = SSL_get_ex_data(ssl,ssl_data_index);
            SSL_SESSION * session = c->cache->cached_session;
            if(session){
                printf("before handshake %p %ld %ld %d\n",session,SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
            }
        }

        SSL_SESSION * session = SSL_get_session(ssl);
        if(session){
            printf("handshake begin %p %p %ld %ld %d\n",ssl,session,SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
        }else{
            printf("handshake begin %p %p \n",ssl,session);
        }
    }

    if(where & SSL_CB_HANDSHAKE_DONE){
        SSL_SESSION * session = SSL_get_session(ssl);
        printf("handshake done %p reused %ld %ld %ld %d\n",session,SSL_session_reused((SSL*)ssl),
                SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
        SSL_SESSION_print_fp(stdout,session);
    }
    */
}

SSL_CTX *ssl_init() {
    SSL_CTX *ctx = NULL;

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    ssl_data_index = SSL_get_ex_new_index(0,0,0,0,0);

    if ((locks = calloc(CRYPTO_num_locks(), sizeof(pthread_mutex_t)))) {
        for (int i = 0; i < CRYPTO_num_locks(); i++) {
            pthread_mutex_init(&locks[i], NULL);
        }

        CRYPTO_set_locking_callback(ssl_lock);
        CRYPTO_set_id_callback(ssl_id);

        if ((ctx = SSL_CTX_new(SSLv23_client_method()))) {
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
            SSL_CTX_set_verify_depth(ctx, 0);
            SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
            SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);
            SSL_CTX_sess_set_new_cb(ctx, new_session_callback);
            SSL_CTX_set_info_callback(ctx, ssl_info_callback);
        }
    }

    return ctx;
}

status ssl_connect(connection *c) {
    int r;
    if(SSL_get_fd(c->ssl)!=c->fd && c->cache && c->cache->cached_session){
        SSL_set_session(c->ssl,c->cache->cached_session);
        //CRYPTO_add(&c->cache->cached_session->references, 1, CRYPTO_LOCK_SSL_SESSION);
        //SSL_SESSION * session = c->cache->cached_session;
        //printf("reuse %p %ld %ld %d\n",session,SSL_SESSION_get_time(session), SSL_SESSION_get_timeout(session),session->references);
    }
    SSL_set_fd(c->ssl, c->fd);
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
    SSL_free(c->ssl);
    c->ssl=NULL;
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
