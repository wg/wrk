include Makefile.in

TARGETDIR := target
BIN := $(TARGETDIR)/wrk

all: $(BIN)

clean:
	$(RM) -rf $(BIN) $(ODIR) $(TARGETDIR)

$(BIN): $(OBJ)
	mkdir -p $(TARGETDIR)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
