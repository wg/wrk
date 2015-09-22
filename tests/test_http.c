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

void
test_append_header_adds_specified_header_to_the_end_of_the_list()
{
	char header1[] = "header1: value";
	char** headers = (char**)malloc(3 * sizeof(char*));

	char** it = headers;
	*it++ = header1;
	*it++ = NULL;
	*it = NULL;

	http_append_header(headers, "header2: value");

	char** second_header = headers + 1;
	assert(equal_strings(*second_header, "header2: value"));

	free(headers);
	headers = NULL;
}

int
main()
{
	test_make_proxy_auth_header_returns_header_with_base64_encoded_auth_data();
	test_append_header_adds_specified_header_to_the_end_of_the_list();

	return 0;
}
