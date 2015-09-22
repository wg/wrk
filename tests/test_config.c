#include <assert.h>
#include <stdbool.h>

#include "../src/config.h"

void
test_proxy_set_returns_true_when_proxy_addr_and_port_are_not_empty()
{
	struct config cfg = {0};
	strcpy(cfg.proxy_addr, "1.2.3.4");
	strcpy(cfg.proxy_port, "8080");

	bool proxy_set = config_proxy_set(&cfg);

	assert(proxy_set == true);
}

void
test_proxy_set_returns_false_when_proxy_addr_and_port_are_empty()
{
	struct config cfg = {0};

	bool proxy_set = config_proxy_set(&cfg);

	assert(proxy_set == false);
}

void
test_proxy_auth_set_returns_false_when_proxy_user_and_password_are_empty()
{
	struct config cfg = {0};

	bool auth_set = config_proxy_auth_set(&cfg);

	assert(auth_set == false);
}

void
test_proxy_auth_set_returns_true_when_proxy_user_and_password_are_set()
{
	struct config cfg = {0};
	strcpy(cfg.proxy_username, "user1");
	strcpy(cfg.proxy_user_password, "password1");

	bool auth_set = config_proxy_auth_set(&cfg);

	assert(auth_set == true);
}

int
main()
{
	test_proxy_set_returns_true_when_proxy_addr_and_port_are_not_empty();
	test_proxy_set_returns_false_when_proxy_addr_and_port_are_empty();
	test_proxy_auth_set_returns_false_when_proxy_user_and_password_are_empty();
	test_proxy_auth_set_returns_true_when_proxy_user_and_password_are_set();

	return 0;
}
