#include <assert.h>

#include "../src/cli_options.h"
#include "../src/config.h"
#include "../src/http_parser.h"
#include "../src/zmalloc.h"

#include "matchers.h"

extern int optind;

static int
parse_config(struct config* cfg, int argc, char* argv[])
{
	char* headers[] = {NULL};
	struct http_parser_url parts = {};
	char* url = NULL;

	optind = 1;
	return parse_args(cfg, &url, &parts, headers, argc, argv);
}


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

static void
test_parse_args_sets_proxy_url_and_port_with_long_option()
{
	struct config cfg;
	char* argv[] = {
		"wrk",
		"--proxy",
		"1.2.3.4:8080",
		"http://google.com",
		"\0"
	};
	int argc = 4;

	parse_config(&cfg, argc, argv);

	assert(equal_strings(cfg.proxy_addr, "1.2.3.4"));
	assert(equal_strings(cfg.proxy_port, "8080"));
}

static void
test_parse_args_sets_proxy_url_and_port_with_short_option()
{
	struct config cfg;
	char* argv[] = {
		"wrk",
		"-x",
		"1.2.3.4:8080",
		"http://google.com",
		"\0"
	};
	int argc = 4;

	parse_config(&cfg, argc, argv);

	assert(equal_strings(cfg.proxy_addr, "1.2.3.4"));
	assert(equal_strings(cfg.proxy_port, "8080"));
}

static void
test_parse_args_sets_proxy_user_and_password_with_long_option()
{
	struct config cfg;
	char* argv[] = {
		"wrk",
		"--proxy-user",
		"user-1:password1",
		"http://google.com",
		"\0"
	};
	int argc = 4;

	parse_config(&cfg, argc, argv);

	assert(equal_strings(cfg.proxy_username, "user-1"));
	assert(equal_strings(cfg.proxy_user_password, "password1"));
}

static void
test_parse_args_sets_proxy_user_and_password_with_short_option()
{
	struct config cfg;
	char* argv[] = {
		"wrk",
		"-U",
		"user-1:password1",
		"http://google.com",
		"\0"
	};
	int argc = 4;

	parse_config(&cfg, argc, argv);

	assert(equal_strings(cfg.proxy_username, "user-1"));
	assert(equal_strings(cfg.proxy_user_password, "password1"));
}

int
main()
{
	test_parse_args_sets_url();
	test_parse_args_sets_proxy_url_and_port_with_long_option();
	test_parse_args_sets_proxy_url_and_port_with_short_option();
	test_parse_args_sets_proxy_user_and_password_with_long_option();
	test_parse_args_sets_proxy_user_and_password_with_short_option();
}
