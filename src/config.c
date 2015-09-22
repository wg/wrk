#include <stdbool.h>

#include "config.h"

static bool
string_empty(const char* str)
{
	return (str == NULL) || (str[0] == '\0');
}

bool
config_proxy_set(const struct config* cfg)
{
	return !string_empty(cfg->proxy_addr) && !string_empty(cfg->proxy_port);
}

bool
config_proxy_auth_set(const struct config* cfg)
{
	return !string_empty(cfg->proxy_username)
		&& !string_empty(cfg->proxy_user_password);
}
