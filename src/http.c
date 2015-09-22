#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "http.h"
#include "base64.h"

char*
http_make_proxy_basic_auth_header(const char* username,
	const char* password)
{
	char* retval = NULL;
	char* auth_data_base64 = NULL;
	char* proxy_auth_header = NULL;

	size_t username_len = strlen(username);
	size_t password_len = strlen(password);

	char* auth_data = (char*)malloc(username_len + password_len + 2);
	if (auth_data == NULL) {
		goto done;
	}

	// Make "username:password" pattern.
	strncpy(auth_data, username, username_len);
	auth_data[username_len] = ':';
	auth_data[username_len + 1] = '\0';
	strncat(auth_data, password, password_len);

	// Encode "username:password" to base64.
	size_t auth_data_len = username_len + password_len + 1;
	auth_data_base64 = base64_encode(auth_data, auth_data_len);
	if (auth_data_base64 == NULL) {
		goto done;
	}

	// Construct proxy auth header.
	char header_prefix[] = "Proxy-Authorization: Basic ";
	proxy_auth_header = (char*)malloc(sizeof(header_prefix)
		+ strlen(auth_data_base64));
	if (proxy_auth_header == NULL) {
		goto done;
	}

	strcpy(proxy_auth_header, header_prefix);
	strcat(proxy_auth_header, auth_data_base64);

	retval = proxy_auth_header;
	proxy_auth_header = NULL;

done:
	if (auth_data != NULL) {
		free(auth_data);
		auth_data = NULL;
	}

	if (auth_data_base64 != NULL) {
		free(auth_data_base64);
		auth_data_base64 = NULL;
	}

	return retval;
}
