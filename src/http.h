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

/**
 * Appends specified header to the header list.
 * Header list must have enough slots allocated for the extra pointer.
 *
 * NOTE: headers data structure should be replaced with something more
 * flexible and safe. Use this function until refactoring is done.
 * Also appending header pointer to the header list, its unclear who is
 * responsible for free'ing up the memory.
 *
 * @param headers array of pointers to HTTP header strings. NULL pointer
 *	terminates the list.
 */
void http_append_header(char** headers, char* header);

#endif /* WRK_HTTP_H */
