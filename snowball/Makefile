include mkinc_utf8.mak
CFLAGS=-O2
snowball_objs=$(patsubst %.c,obj/%.o,$(notdir $(snowball_sources)))

all: lib/libstemmer.a
lib/libstemmer.a: $(snowball_objs) | lib
	$(AR) -cr $@ $^

clean:
	rm -rf  lib obj

obj/%.o: */%.c | obj
	$(CC) $(CFLAGS) -c -o $@ $<

lib:
	mkdir -p $@

obj:
	mkdir -p $@
