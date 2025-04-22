BIN=./build
BUILD=$(BIN)/build
SNB_INC=./snowball/include
SNB_LIB=./snowball/lib/libstemmer.a
PLATFORM_SRC=glibc_log.c glibc_platform.c glibc_platform.h
CXX=g++
CXXFLAGS=-Wall -Wextra -fno-exceptions -ggdb
.PHONY: snowball run

$(BIN)/build: main.c $(PLATFORM_SRC) snowball
	$(CXX) $(CXXFLAGS) -I $(SNB_INC) -o $@ $< $(SNB_LIB)

run: $(BIN)/build
	./$(BIN)/build

snowball:
	$(MAKE) -C $@
