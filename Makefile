exe = ceeteef
man-cmd = $(exe).1
man-api = ctf.3
library = lib/libctf.h
library-89 = lib/libc89tf.h
bin-dir = /usr/local/bin
man-dir-1 = /usr/local/share/man/man1
man-dir-3 = /usr/local/share/man/man3
library-dir = /usr/local/include
c-flags = -Ilib -std=c99 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200112L \
	$(CFLAGS)
linkage = -ldl

all: $(exe)

$(exe): src lib
	$(CC) $(c-flags) -o $(exe) src/*.c $(linkage)

install: $(exe) $(library) $(library-89)
	cp $(exe) $(bin-dir)/
	mkdir -p $(man-dir-1) $(man-dir-3)
	cp $(man-cmd) $(man-dir-1)/
	cp $(man-api) $(man-dir-3)/
	cp $(library) $(library-dir)/
	cp $(library-89) $(library-dir)/

clean:
	$(RM) $(exe)
