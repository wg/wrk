CFLAGS  += -std=c99 -Wall -O2 -D_REENTRANT -fPIC
LIBS    := -lm -lssl -lcrypto -lpthread

TARGET  := $(shell uname -s | tr '[A-Z]' '[a-z]' 2>/dev/null || echo unknown)

ifeq ($(TARGET), sunos)
	CFLAGS += -D_PTHREADS -D_POSIX_C_SOURCE=200112L
	LIBS   += -lsocket
else ifeq ($(TARGET), darwin)
	export MACOSX_DEPLOYMENT_TARGET = $(shell sw_vers -productVersion)
else ifeq ($(TARGET), linux)
	CFLAGS  += -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -D_DEFAULT_SOURCE
	LIBS    += -ldl
	LDFLAGS += -Wl,-E
else ifeq ($(TARGET), freebsd)
	CFLAGS  += -D_DECLARE_C99_LDBL_MATH
	LDFLAGS += -Wl,-E
endif

SRC  := main.c wrk.c net.c ssl.c aprintf.c stats.c \
		units.c ae.c zmalloc.c http_parser.c
BIN  := wrk
SHARED := wrk.so
VER  ?= $(shell git describe --tags --always --dirty)

ODIR := obj
OBJ  := $(patsubst %.c,$(ODIR)/%.o,$(SRC)) $(ODIR)/version.o

DEPS    :=
CFLAGS  += -I$(ODIR)/include
LDFLAGS += -L$(ODIR)/lib

ifneq ($(WITH_OPENSSL),)
	CFLAGS  += -I$(WITH_OPENSSL)/include
	LDFLAGS += -L$(WITH_OPENSSL)/lib
else
	DEPS += $(ODIR)/lib/libssl.a
endif

all: $(BIN)

clean:
	$(RM) -rf $(BIN) $(SHARED) obj/*

$(BIN): $(OBJ)
	@echo LINK $(BIN)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(SHARED): $(OBJ)
	@echo LINK $(SHARED)
	@$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)

$(OBJ): config.h Makefile $(DEPS) | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(ODIR)/version.o:
	@echo 'const char *VERSION="$(VER)";' | $(CC) -xc -c -o $@ -

$(ODIR)/%.o : %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies

OPENSSL := $(notdir $(patsubst %.tar.gz,%,$(wildcard deps/openssl*.tar.gz)))

OPENSSL_OPTS = no-shared no-psk no-srp no-dtls no-idea --prefix=$(abspath $(ODIR))

$(ODIR)/$(OPENSSL): deps/$(OPENSSL).tar.gz | $(ODIR)
	@tar -C $(ODIR) -xf $<

$(ODIR)/lib/libssl.a: $(ODIR)/$(OPENSSL)
	@echo Building OpenSSL...
	@$(SHELL) -c "cd $< && ./config $(OPENSSL_OPTS)"
	@$(MAKE) -C $< depend
	@$(MAKE) -C $<
	@$(MAKE) -C $< install_sw
	@touch $@

# ------------

.PHONY: all clean
.PHONY: $(ODIR)/version.o

.SUFFIXES:
.SUFFIXES: .c .o

vpath %.c  src
vpath %.h  src
