C_TARGET := websocket-client
S_TARGET := websocket-server

DEPSDIR := $(PWD)/deps

CFLAG := -I$(DEPSDIR)/include
LDFLAG := -L$(DEPSDIR)/lib -lwebsockets -lpthread

export DEPSDIR := $(PWD)/deps

.PHONY: $(S_TARGET) $(C_TARGET) prepare clean distclean

all: $(S_TARGET) $(C_TARGET)

prepare:
	$(MAKE) -C $(DEPSDIR)

$(C_TARGET):
	$(CC) websocket_client.c -o $@ $(CFLAG) $(LDFLAG)

$(S_TARGET):
	$(CC) websocket_server.c -o $@ $(CFLAG) $(LDFLAG)

clean:
	rm -r $(S_TARGET) $(C_TARGET)

distclean:
	$(MAKE) -C $(DEPSDIR) distclean
	rm -r $(S_TARGET) $(C_TARGET)