BIN=./build
PLATFORM_SRC=glibc_log.c glibc_platform.c glibc_platform.h
CXX=g++
CXXFLAGS=-Wall -Wextra -fno-exceptions -ggdb

.PHONY: snowball

$(BIN)/build: main.c $(PLATFORM_SRC) snowball
	$(CXX) $(CXXFLAGS) -I ./snowball/include/ -o $@ $< snowball/lib/libstemmer.a

snowball:
	$(MAKE) -C $@
