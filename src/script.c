// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <string.h>
#include "script.h"

lua_State *script_create(char *scheme, char *host, int port, char *path) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_dostring(L, "wrk = require \"wrk\"");

    lua_getglobal(L, "wrk");
    lua_pushstring(L, scheme);
    lua_pushstring(L, host);
    lua_pushinteger(L, port);
    lua_pushstring(L, path);
    lua_setfield(L, 1, "path");
    lua_setfield(L, 1, "port");
    lua_setfield(L, 1, "host");
    lua_setfield(L, 1, "scheme");
    lua_pop(L, 1);

    return L;
}

void script_headers(lua_State *L, char **headers) {
    lua_getglobal(L, "wrk");
    lua_getfield(L, 1, "headers");
    for (char **h = headers; *h; h++) {
        char *p = strchr(*h, ':');
        if (p && p[1] == ' ') {
            lua_pushlstring(L, *h, p - *h);
            lua_pushstring(L, p + 2);
            lua_settable(L, 2);
        }
    }
    lua_pop(L, 2);
}

void script_init(lua_State *L, char *script) {
    if (script && luaL_dofile(L, script)) {
        const char *cause = lua_tostring(L, -1);
        fprintf(stderr, "script %s failed: %s", script, cause);
    }

    lua_getglobal(L, "init");
    lua_call(L, 0, 0);
}

void script_request(lua_State *L, char **buf, size_t *len) {
    lua_getglobal(L, "request");
    lua_call(L, 0, 1);
    *buf = (char *) lua_tostring(L, 1);
    *len = (size_t) lua_strlen(L, 1);
    lua_pop(L, 1);
}

bool script_is_static(lua_State *L) {
    lua_getglobal(L, "wrk");
    lua_getfield(L, 1, "request");
    lua_getglobal(L, "request");
    bool is_static = lua_equal(L, 2, 3);
    lua_pop(L, 3);
    return is_static;
}
