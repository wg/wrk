#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <unistd.h>
#include "stats.h"
#include "wrk.h"

lua_State *script_create(char *, char *, char **);

bool script_resolve(lua_State *, char *, char *);
void script_setup(lua_State *, thread *);
void script_done(lua_State *, stats *, stats *);

void script_init(lua_State *, thread *, int, char **);
void script_request(lua_State *, char **, size_t *);
void script_response(lua_State *, int, buffer *, buffer *);
size_t script_verify_request(lua_State *L);

bool script_is_static(lua_State *);
bool script_want_response(lua_State *L);
bool script_has_done(lua_State *L);
void script_summary(lua_State *, uint64_t, uint64_t, uint64_t);
void script_errors(lua_State *, errors *);

void script_copy_value(lua_State *, lua_State *, int);
int script_parse_url(char *, struct http_parser_url *);

void buffer_append(buffer *, const char *, size_t);
void buffer_reset(buffer *);
char *buffer_pushlstring(lua_State *, char *);

#endif /* SCRIPT_H */
