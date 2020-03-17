
WSDIR := $(DEPSDIR)/libwebsockets

.PHONY: websockets install

all: install

websockets:
	cd $(WSDIR) \
	&& mkdir -p build \
	&& cd build \
	&& cmake .. \
	&& make

install: websockets
	mkdir -p $(DEPSDIR)/include
	mkdir -p $(DEPSDIR)/lib
	cp -rf $(DEPSDIR)/libwebsockets/build/include/* $(CUR_MAKEFILE_DIR)/include
	cp -rf $(DEPSDIR)/libwebsockets/build/lib/* $(CUR_MAKEFILE_DIR)/lib

distclean:
	@rm -rf $(DEPSDIR)/include
	@rm -rf $(DEPSDIR)/lib