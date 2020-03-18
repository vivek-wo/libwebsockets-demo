
ifdef DEPSDIR

WSDIR := $(DEPSDIR)/libwebsockets

.PHONY: websockets install

all: install

websockets:
	cd $(WSDIR) \
	&& mkdir -p build \
	&& cd build \
	&& cmake .. -DLWS_WITH_HTTP2=1 \
	-DLWS_WITH_SSL=0 \
	&& make

install: websockets
	mkdir -p $(DEPSDIR)/include
	mkdir -p $(DEPSDIR)/lib
	cp -rf $(WSDIR)/build/include/* $(DEPSDIR)/include
	cp -rf $(WSDIR)/build/lib/* $(DEPSDIR)/lib

distclean:
	rm -rf $(DEPSDIR)/include
	rm -rf $(DEPSDIR)/lib
	rm -rf $(WSDIR)/build

endif