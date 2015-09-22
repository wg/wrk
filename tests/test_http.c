#include <assert.h>
#include <stdlib.h>

#include "../src/http.h"

#include "matchers.h"


void
test_make_proxy_auth_header_returns_header_with_base64_encoded_auth_data()
{
	char* header = http_make_proxy_basic_auth_header("user1", "password1");

	assert(equal_strings(header,
		"Proxy-Authorization: Basic dXNlcjE6cGFzc3dvcmQx"));

	free(header);
	header = NULL;
}

int
main()
{
	test_make_proxy_auth_header_returns_header_with_base64_encoded_auth_data();

	return 0;
}
