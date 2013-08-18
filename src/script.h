#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "stats.h"

lua_State *script_create(char *, char *, char *, char *);
void script_headers(lua_State *, char **);

void script_init(lua_State *, char *, int, char **);
void script_done(lua_State *, stats *, stats *);
void script_request(lua_State *, char **, size_t *);

bool script_is_static(lua_State *);
bool script_has_done(lua_State *L);
void script_summary(lua_State *, uint64_t, uint64_t, uint64_t);
void script_errors(lua_State *, errors *);

#endif /* SCRIPT_H */
