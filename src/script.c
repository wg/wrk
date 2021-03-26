// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <stdlib.h>
#include <string.h>
#include "script.h"
#include "http_parser.h"
#include "zmalloc.h"

typedef struct {
    char *name;
    int   type;
    void *value;
} table_field;

static int script_addr_tostring(lua_State *);
static int script_addr_gc(lua_State *);
static int script_stats_call(lua_State *);
static int script_stats_len(lua_State *);
static int script_stats_index(lua_State *);
static int script_thread_index(lua_State *);
static int script_thread_newindex(lua_State *);
static int script_wrk_lookup(lua_State *);
static int script_wrk_connect(lua_State *);

static void set_fields(lua_State *, int, const table_field *);
static void set_field(lua_State *, int, char *, int);
static int push_url_part(lua_State *, char *, struct http_parser_url *, enum http_parser_url_fields);

static const struct luaL_Reg addrlib[] = {
    { "__tostring", script_addr_tostring   },
    { "__gc"    ,   script_addr_gc         },
    { NULL,         NULL                   }
};

static const struct luaL_Reg statslib[] = {
    { "__call",     script_stats_call      },
    { "__index",    script_stats_index     },
    { "__len",      script_stats_len       },
    { NULL,         NULL                   }
};

static const struct luaL_Reg threadlib[] = {
    { "__index",    script_thread_index    },
    { "__newindex", script_thread_newindex },
    { NULL,         NULL                   }
};

lua_State *script_create(char *file, char *url, char **headers) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    (void) luaL_dostring(L, "wrk = require \"wrk\"");

    luaL_newmetatable(L, "wrk.addr");
    luaL_register(L, NULL, addrlib);
    luaL_newmetatable(L, "wrk.stats");
    luaL_register(L, NULL, statslib);
    luaL_newmetatable(L, "wrk.thread");
    luaL_register(L, NULL, threadlib);

    struct http_parser_url parts = {};
    script_parse_url(url, &parts);
    char *path = "/";

    if (parts.field_set & (1 << UF_PATH)) {
        path = &url[parts.field_data[UF_PATH].off];
    }

    const table_field fields[] = {
        { "lookup",  LUA_TFUNCTION, script_wrk_lookup  },
        { "connect", LUA_TFUNCTION, script_wrk_connect },
        { "path",    LUA_TSTRING,   path               },
        { NULL,      0,             NULL               },
    };

    lua_getglobal(L, "wrk");

    set_field(L, 4, "scheme", push_url_part(L, url, &parts, UF_SCHEMA));
    set_field(L, 4, "host",   push_url_part(L, url, &parts, UF_HOST));
    set_field(L, 4, "port",   push_url_part(L, url, &parts, UF_PORT));
    set_fields(L, 4, fields);

    lua_getfield(L, 4, "headers");
    for (char **h = headers; *h; h++) {
        char *p = strchr(*h, ':');
        if (p && p[1] == ' ') {
            lua_pushlstring(L, *h, p - *h);
            lua_pushstring(L, p + 2);
            lua_settable(L, 5);
        }
    }
    lua_pop(L, 5);

    if (file && luaL_dofile(L, file)) {
        const char *cause = lua_tostring(L, -1);
        fprintf(stderr, "%s: %s\n", file, cause);
    }

    return L;
}

bool script_resolve(lua_State *L, char *host, char *service) {
    lua_getglobal(L, "wrk");

    lua_getfield(L, -1, "resolve");
    lua_pushstring(L, host);
    lua_pushstring(L, service);
    lua_call(L, 2, 0);

    lua_getfield(L, -1, "addrs");
    size_t count = lua_objlen(L, -1);
    lua_pop(L, 2);
    return count > 0;
}

void script_push_thread(lua_State *L, thread *t) {
    thread **ptr = (thread **) lua_newuserdata(L, sizeof(thread **));
    *ptr = t;
    luaL_getmetatable(L, "wrk.thread");
    lua_setmetatable(L, -2);
}

void script_init(lua_State *L, thread *t, int argc, char **argv) {
    lua_getglobal(t->L, "wrk");

    script_push_thread(t->L, t);
    lua_setfield(t->L, -2, "thread");

    lua_getglobal(L, "wrk");
    lua_getfield(L, -1, "setup");
    script_push_thread(L, t);
    lua_call(L, 1, 0);
    lua_pop(L, 1);

    lua_getfield(t->L, -1, "init");
    lua_newtable(t->L);
    for (int i = 0; i < argc; i++) {
        lua_pushstring(t->L, argv[i]);
        lua_rawseti(t->L, -2, i);
    }
    lua_call(t->L, 1, 0);
    lua_pop(t->L, 1);
}

uint64_t script_delay(lua_State *L) {
    lua_getglobal(L, "delay");
    lua_call(L, 0, 1);
    uint64_t delay = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return delay;
}

void script_request(lua_State *L, char **buf, size_t *len) {
    int pop = 1;
    lua_getglobal(L, "request");
    if (!lua_isfunction(L, -1)) {
        lua_getglobal(L, "wrk");
        lua_getfield(L, -1, "request");
        pop += 2;
    }
    lua_call(L, 0, 1);
    const char *str = lua_tolstring(L, -1, len);
    *buf = realloc(*buf, *len);
    memcpy(*buf, str, *len);
    lua_pop(L, pop);
}

void script_response(lua_State *L, int status, buffer *headers, buffer *body) {
    lua_getglobal(L, "response");
    lua_pushinteger(L, status);
    lua_newtable(L);

    for (char *c = headers->buffer; c < headers->cursor; ) {
        c = buffer_pushlstring(L, c);
        c = buffer_pushlstring(L, c);
        lua_rawset(L, -3);
    }

    lua_pushlstring(L, body->buffer, body->cursor - body->buffer);
    lua_call(L, 3, 0);

    buffer_reset(headers);
    buffer_reset(body);
}

bool script_is_function(lua_State *L, char *name) {
    lua_getglobal(L, name);
    bool is_function = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return is_function;
}

bool script_is_static(lua_State *L) {
    return !script_is_function(L, "request");
}

bool script_want_response(lua_State *L) {
    return script_is_function(L, "response");
}

bool script_has_delay(lua_State *L) {
    return script_is_function(L, "delay");
}

bool script_has_done(lua_State *L) {
    return script_is_function(L, "done");
}

void script_header_done(lua_State *L, luaL_Buffer *buffer) {
    luaL_pushresult(buffer);
}

void script_summary(lua_State *L, uint64_t duration, uint64_t requests, uint64_t bytes) {
    const table_field fields[] = {
        { "duration", LUA_TNUMBER, &duration },
        { "requests", LUA_TNUMBER, &requests },
        { "bytes",    LUA_TNUMBER, &bytes    },
        { NULL,       0,           NULL      },
    };
    lua_newtable(L);
    set_fields(L, 1, fields);
}

void script_errors(lua_State *L, errors *errors) {
    uint64_t e[] = {
        errors->connect,
        errors->read,
        errors->write,
        errors->status,
        errors->timeout
    };
    const table_field fields[] = {
        { "connect", LUA_TNUMBER, &e[0] },
        { "read",    LUA_TNUMBER, &e[1] },
        { "write",   LUA_TNUMBER, &e[2] },
        { "status",  LUA_TNUMBER, &e[3] },
        { "timeout", LUA_TNUMBER, &e[4] },
        { NULL,      0,           NULL  },
    };
    lua_newtable(L);
    set_fields(L, 2, fields);
    lua_setfield(L, 1, "errors");
}

void script_push_stats(lua_State *L, stats *s) {
    stats **ptr = (stats **) lua_newuserdata(L, sizeof(stats **));
    *ptr = s;
    luaL_getmetatable(L, "wrk.stats");
    lua_setmetatable(L, -2);
}

void script_done(lua_State *L, stats *latency, stats *requests) {
    lua_getglobal(L, "done");
    lua_pushvalue(L, 1);

    script_push_stats(L, latency);
    script_push_stats(L, requests);

    lua_call(L, 3, 0);
    lua_pop(L, 1);
}

static int verify_request(http_parser *parser) {
    size_t *count = parser->data;
    (*count)++;
    return 0;
}

size_t script_verify_request(lua_State *L) {
    http_parser_settings settings = {
        .on_message_complete = verify_request
    };
    http_parser parser;
    char *request = NULL;
    size_t len, count = 0;

    script_request(L, &request, &len);
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = &count;

    size_t parsed = http_parser_execute(&parser, &settings, request, len);

    if (parsed != len || count == 0) {
        enum http_errno err = HTTP_PARSER_ERRNO(&parser);
        const char *desc = http_errno_description(err);
        const char *msg = err != HPE_OK ? desc : "incomplete request";
        int line = 1, column = 1;

        for (char *c = request; c < request + parsed; c++) {
            column++;
            if (*c == '\n') {
                column = 1;
                line++;
            }
        }

        fprintf(stderr, "%s at %d:%d\n", msg, line, column);
        exit(1);
    }

    return count;
}

static struct addrinfo *checkaddr(lua_State *L) {
    struct addrinfo *addr = luaL_checkudata(L, -1, "wrk.addr");
    luaL_argcheck(L, addr != NULL, 1, "`addr' expected");
    return addr;
}

void script_addr_copy(struct addrinfo *src, struct addrinfo *dst) {
    *dst = *src;
    dst->ai_addr = zmalloc(src->ai_addrlen);
    memcpy(dst->ai_addr, src->ai_addr, src->ai_addrlen);
}

struct addrinfo *script_addr_clone(lua_State *L, struct addrinfo *addr) {
    struct addrinfo *udata = lua_newuserdata(L, sizeof(*udata));
    luaL_getmetatable(L, "wrk.addr");
    lua_setmetatable(L, -2);
    script_addr_copy(addr, udata);
    return udata;
}

static int script_addr_tostring(lua_State *L) {
    struct addrinfo *addr = checkaddr(L);
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    int flags = NI_NUMERICHOST | NI_NUMERICSERV;
    int rc = getnameinfo(addr->ai_addr, addr->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, flags);
    if (rc != 0) {
        const char *msg = gai_strerror(rc);
        return luaL_error(L, "addr tostring failed %s", msg);
    }

    lua_pushfstring(L, "%s:%s", host, service);
    return 1;
}

static int script_addr_gc(lua_State *L) {
    struct addrinfo *addr = checkaddr(L);
    zfree(addr->ai_addr);
    return 0;
}

static stats *checkstats(lua_State *L) {
    stats **s = luaL_checkudata(L, 1, "wrk.stats");
    luaL_argcheck(L, s != NULL, 1, "`stats' expected");
    return *s;
}

static int script_stats_percentile(lua_State *L) {
    stats *s = checkstats(L);
    lua_Number p = luaL_checknumber(L, 2);
    lua_pushnumber(L, stats_percentile(s, p));
    return 1;
}

static int script_stats_call(lua_State *L) {
    stats *s = checkstats(L);
    uint64_t index = lua_tonumber(L, 2);
    uint64_t count;
    lua_pushnumber(L, stats_value_at(s, index - 1, &count));
    lua_pushnumber(L, count);
    return 2;
}

static int script_stats_index(lua_State *L) {
    stats *s = checkstats(L);
    const char *method = lua_tostring(L, 2);
    if (!strcmp("min",   method)) lua_pushnumber(L, s->min);
    if (!strcmp("max",   method)) lua_pushnumber(L, s->max);
    if (!strcmp("mean",  method)) lua_pushnumber(L, stats_mean(s));
    if (!strcmp("stdev", method)) lua_pushnumber(L, stats_stdev(s, stats_mean(s)));
    if (!strcmp("percentile", method)) {
        lua_pushcfunction(L, script_stats_percentile);
    }
    return 1;
}

static int script_stats_len(lua_State *L) {
    stats *s = checkstats(L);
    lua_pushinteger(L, stats_popcount(s));
    return 1;
}

static thread *checkthread(lua_State *L) {
    thread **t = luaL_checkudata(L, 1, "wrk.thread");
    luaL_argcheck(L, t != NULL, 1, "`thread' expected");
    return *t;
}

static int script_thread_get(lua_State *L) {
    thread *t = checkthread(L);
    const char *key = lua_tostring(L, -1);
    lua_getglobal(t->L, key);
    script_copy_value(t->L, L, -1);
    lua_pop(t->L, 1);
    return 1;
}

static int script_thread_set(lua_State *L) {
    thread *t = checkthread(L);
    const char *name = lua_tostring(L, -2);
    script_copy_value(L, t->L, -1);
    lua_setglobal(t->L, name);
    return 0;
}

static int script_thread_stop(lua_State *L) {
    thread *t = checkthread(L);
    aeStop(t->loop);
    return 0;
}

static int script_thread_index(lua_State *L) {
    thread *t = checkthread(L);
    const char *key = lua_tostring(L, 2);
    if (!strcmp("get",  key)) lua_pushcfunction(L, script_thread_get);
    if (!strcmp("set",  key)) lua_pushcfunction(L, script_thread_set);
    if (!strcmp("stop", key)) lua_pushcfunction(L, script_thread_stop);
    if (!strcmp("addr", key)) script_addr_clone(L, t->addr);
    return 1;
}

static int script_thread_newindex(lua_State *L) {
    thread *t = checkthread(L);
    const char *key = lua_tostring(L, -2);
    if (!strcmp("addr", key)) {
        struct addrinfo *addr = checkaddr(L);
        if (t->addr) zfree(t->addr->ai_addr);
        t->addr = zrealloc(t->addr, sizeof(*addr));
        script_addr_copy(addr, t->addr);
    } else {
        luaL_error(L, "cannot set '%s' on thread", luaL_typename(L, -1));
    }
    return 0;
}

static int script_wrk_lookup(lua_State *L) {
    struct addrinfo *addrs;
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    int rc, index = 1;

    const char *host    = lua_tostring(L, -2);
    const char *service = lua_tostring(L, -1);

    if ((rc = getaddrinfo(host, service, &hints, &addrs)) != 0) {
        const char *msg = gai_strerror(rc);
        fprintf(stderr, "unable to resolve %s:%s %s\n", host, service, msg);
        exit(1);
    }

    lua_newtable(L);
    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        script_addr_clone(L, addr);
        lua_rawseti(L, -2, index++);
    }

    freeaddrinfo(addrs);
    return 1;
}

static int script_wrk_connect(lua_State *L) {
    struct addrinfo *addr = checkaddr(L);
    int fd, connected = 0;
    if ((fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) != -1) {
        connected = connect(fd, addr->ai_addr, addr->ai_addrlen) == 0;
        close(fd);
    }
    lua_pushboolean(L, connected);
    return 1;
}

void script_copy_value(lua_State *src, lua_State *dst, int index) {
    switch (lua_type(src, index)) {
        case LUA_TBOOLEAN:
            lua_pushboolean(dst, lua_toboolean(src, index));
            break;
        case LUA_TNIL:
            lua_pushnil(dst);
            break;
        case LUA_TNUMBER:
            lua_pushnumber(dst, lua_tonumber(src, index));
            break;
        case LUA_TSTRING:
            lua_pushstring(dst, lua_tostring(src, index));
            break;
        case LUA_TTABLE:
            lua_newtable(dst);
            lua_pushnil(src);
            while (lua_next(src, index - 1)) {
                script_copy_value(src, dst, -2);
                script_copy_value(src, dst, -1);
                lua_settable(dst, -3);
                lua_pop(src, 1);
            }
            lua_pop(src, 1);
            break;
        default:
            luaL_error(src, "cannot transfer '%s' to thread", luaL_typename(src, index));
    }
}

int script_parse_url(char *url, struct http_parser_url *parts) {
    if (!http_parser_parse_url(url, strlen(url), 0, parts)) {
        if (!(parts->field_set & (1 << UF_SCHEMA))) return 0;
        if (!(parts->field_set & (1 << UF_HOST)))   return 0;
        return 1;
    }
    return 0;
}

static int push_url_part(lua_State *L, char *url, struct http_parser_url *parts, enum http_parser_url_fields field) {
    int type = parts->field_set & (1 << field) ? LUA_TSTRING : LUA_TNIL;
    uint16_t off, len;
    switch (type) {
        case LUA_TSTRING:
            off = parts->field_data[field].off;
            len = parts->field_data[field].len;
            lua_pushlstring(L, &url[off], len);
            break;
        case LUA_TNIL:
            lua_pushnil(L);
    }
    return type;
}

static void set_field(lua_State *L, int index, char *field, int type) {
    (void) type;
    lua_setfield(L, index, field);
}

static void set_fields(lua_State *L, int index, const table_field *fields) {
    for (int i = 0; fields[i].name; i++) {
        table_field f = fields[i];
        switch (f.value == NULL ? LUA_TNIL : f.type) {
            case LUA_TFUNCTION:
                lua_pushcfunction(L, (lua_CFunction) f.value);
                break;
            case LUA_TNUMBER:
                lua_pushinteger(L, *((lua_Integer *) f.value));
                break;
            case LUA_TSTRING:
                lua_pushstring(L, (const char *) f.value);
                break;
            case LUA_TNIL:
                lua_pushnil(L);
                break;
        }
        lua_setfield(L, index, f.name);
    }
}

void buffer_append(buffer *b, const char *data, size_t len) {
    size_t used = b->cursor - b->buffer;
    while (used + len + 1 >= b->length) {
        b->length += 1024;
        b->buffer  = realloc(b->buffer, b->length);
        b->cursor  = b->buffer + used;
    }
    memcpy(b->cursor, data, len);
    b->cursor += len;
}

void buffer_reset(buffer *b) {
    b->cursor = b->buffer;
}

char *buffer_pushlstring(lua_State *L, char *start) {
    char *end = strchr(start, 0);
    lua_pushlstring(L, start, end - start);
    return end + 1;
}
