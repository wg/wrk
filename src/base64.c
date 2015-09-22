#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

char*
base64_encode(const char* buff, size_t len)
{
	BUF_MEM *buff_ptr = NULL;

	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* bmem = BIO_new(BIO_s_mem());
	BIO* bio = BIO_push(b64, bmem);

	// Ignore newlines - write everything in one line.
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio, buff, len);
	BIO_flush(bio);

	BIO_get_mem_ptr(bio, &buff_ptr);
	BIO_set_close(bio, BIO_NOCLOSE);
	BIO_free_all(bio);

	char *encoded_buff = (char *)malloc(buff_ptr->length);
	if (encoded_buff == NULL) {
		return NULL;
	}

	memcpy(encoded_buff, buff_ptr->data, buff_ptr->length);

	return encoded_buff;
}
