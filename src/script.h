#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

lua_State *script_create(char *, char *, int, char *);
void script_headers(lua_State *, char **);
void script_init(lua_State *, char *);
void script_request(lua_State *, char **, size_t *);
bool script_is_static(lua_State *);

#endif /* SCRIPT_H */
