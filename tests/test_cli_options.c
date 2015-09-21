#include <assert.h>

#include "../src/cli_options.h"
#include "../src/config.h"
#include "../src/http_parser.h"

#include "matchers.h"

extern int optind;

static void
test_parse_args_sets_url()
{
	struct config cfg;
	char* argv[] = {
		"wrk",
		"http://google.com",
		"\0"
	};
	int argc = 2;

	char* headers[] = {NULL};
	struct http_parser_url parts = {};
	char* url = NULL;

	optind = 1;
	parse_args(&cfg, &url, &parts, headers, argc, argv);

	assert(equal_strings(url, "http://google.com"));
}

int
main()
{
	test_parse_args_sets_url();
}
