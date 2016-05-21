// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <pthread.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "zmalloc.h"

#include "ssl.h"

static pthread_mutex_t *locks;

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

SSL_CTX *ssl_init(char* ssl_cert) {
    SSL_CTX *ctx = NULL;

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

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
	    if(ssl_cert) {
		FILE *fp;
		long file_size;
		char *buffer;
		fp = fopen (ssl_cert , "rb");
		if(!fp) {
			fprintf(stderr, "can't open cert file\n");
			return 0L;
		}

		fseek(fp , 0L , SEEK_END);
		file_size = ftell(fp);
		rewind( fp );
		printf("file size: %ld\n",file_size);
		buffer = zmalloc(file_size + 1);
		if(!buffer) {
			fprintf(stderr, "can't create buffer\n");
			fclose(fp);
			free(buffer);
			return 0L;
		}
		if(fread(buffer, file_size, 1, fp) != 1 ) {
			fprintf(stderr, "can't read cert file\n");
			fclose(fp);
			free(buffer);
			return 0L;
		}
		fclose(fp);

		char * kstart = strstr(buffer, "RSA P");
		if (kstart == 0L) {
			return 0L;
		}

    		kstart -= 11;
		unsigned int cert_len = (kstart - buffer);

		char* cert_txt = zmalloc(cert_len+1);
		strncpy(cert_txt, buffer, cert_len);
		cert_txt[cert_len] = '\0';
		char* key = kstart;

		X509 *cert = NULL;
		RSA *rsa = NULL;
                BIO *bio;
		BIO *kbio = NULL;

		bio = BIO_new_mem_buf(cert_txt, -1);
		if(bio == NULL) {
			fprintf(stderr, "BIO_new_mem_buf failed\n");
			return 0L;
		}

		cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
		if(cert == NULL) {
			fprintf(stderr, "PEM_read_bio_X509 failed\n");
			return 0L;
		}

		int ret = SSL_CTX_use_certificate(ctx, cert);
		if(ret != 1) {
			fprintf(stderr, "can't use certificate\n");
			return 0L;
		}

		kbio = BIO_new_mem_buf(key, -1);
		if(kbio == NULL) {
			fprintf(stderr, "BIO_new_mem_buf failed\n");
			return 0L;
		}

		rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
		if(rsa == NULL) {
			fprintf(stderr, "PEM_read_bio_RSAPrivateKey failed\n");
    		}


		ret = SSL_CTX_use_RSAPrivateKey(ctx, rsa);
		if(ret != 1) {
			fprintf(stderr, "SSL_CTX_use_RSAPrivateKey failed\n");
			return 0L;
		}

		if(bio)
			BIO_free(bio);

		if(kbio)
			BIO_free(kbio);

		if(rsa)
			RSA_free(rsa);

		if(cert)
			X509_free(cert);

		zfree(buffer);

	    }
        }
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
