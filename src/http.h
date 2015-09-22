#ifndef WRK_HTTP_H
#define WRK_HTTP_H


/**
 * Constructs HTTP proxy basic authorization header. E.g.
 *	Proxy-Authorization: Basic 98sdj90zxxcZZ==
 *
 * You must free() the returned header string.
 *
 * @return pointer to constructed header on success. NULL on error.
 */
char* http_make_proxy_basic_auth_header(const char* username,
	const char* password);

#endif /* WRK_HTTP_H */
