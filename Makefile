BDEPSLIBS = hiredis jemalloc linenoise lua

.PHONY : all

default: all

all:
	cd deps && $(MAKE) $(DEPSLIBS)
	cd src && $(MAKE)
