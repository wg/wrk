CFLAGS  := -std=c99 -Wall -O2 -D_REENTRANT
LIBS    := -lpthread -lm -lcrypto -lssl

TARGET  := $(shell uname -s | tr '[A-Z]' '[a-z]' 2>/dev/null || echo unknown)

ifeq ($(TARGET), sunos)
	CFLAGS += -D_PTHREADS -D_POSIX_C_SOURCE=200112L
	LIBS   += -lsocket
else ifeq ($(TARGET), darwin)
	LDFLAGS += -pagezero_size 10000 -image_base 100000000
else ifeq ($(TARGET), linux)
	CFLAGS  += -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE
	LIBS    += -ldl
	LDFLAGS += -Wl,-E
else ifeq ($(TARGET), freebsd)
	CFLAGS  += -D_DECLARE_C99_LDBL_MATH
	LDFLAGS += -Wl,-E
endif

SRC  := wrk.c net.c ssl.c aprintf.c stats.c script.c units.c \
		ae.c zmalloc.c http_parser.c cli_options.c config.c
SRC_FILES := $(addprefix src/, $(filter-out wrk.c, $(SRC)) )

TEST_FILES := $(shell find tests -name 'test_*.c')
TEST_FILES := $(notdir $(TEST_FILES))
TEST_SUITES := $(basename $(TEST_FILES))

BIN  := wrk

ODIR := obj
OBJ  := $(patsubst %.c,$(ODIR)/%.o,$(SRC)) $(ODIR)/bytecode.o

LDIR     = deps/luajit/src
LIBS    := -lluajit $(LIBS)
CFLAGS  += -I$(LDIR)
LDFLAGS += -L$(LDIR)

all: $(BIN)

clean:
	$(RM) $(BIN) obj/*
	@$(MAKE) -C deps/luajit clean

$(BIN): $(OBJ)
	@echo LINK $(BIN)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ): config.h Makefile $(LDIR)/libluajit.a | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(ODIR)/bytecode.o: src/wrk.lua
	@echo LUAJIT $<
	@$(SHELL) -c 'cd $(LDIR) && ./luajit -b $(CURDIR)/$< $(CURDIR)/$@'

$(ODIR)/%.o : %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(LDIR)/libluajit.a:
	@echo Building LuaJIT...
	@$(MAKE) -C $(LDIR) BUILDMODE=static

test:
	mkdir -p build/tests
	for test_suite in $(TEST_SUITES) ; do $(MAKE) $$test_suite ; done
.PHONY: test

test_%:
	$(CC) $(CFLAGS) tests/$@.c $(SRC_FILES) $(LIBS) $(LDFLAGS) \
		-o build/tests/$@ -g

	build/tests/$@
.PHONY: test_%

.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .c .o .lua

vpath %.c   src
vpath %.h   src
vpath %.lua scripts
