exe = ceeteef
man-cmd = $(exe).1
man-api = ctf.3
library = lib/libctf.h
bin-dir = /usr/local/bin
man-dir-1 = /usr/local/share/man/man1
man-dir-3 = /usr/local/share/man/man3
library-dir = /usr/local/include
c-flags = -Ilib -std=c99 -Wall -Wextra -Wpedantic $(CFLAGS)

all: $(exe)

$(exe): src lib
	$(CC) $(c-flags) -o $(exe) src/*.c

install: $(exe) $(library)
	cp $(exe) $(bin-dir)/
	cp $(man-cmd) $(man-dir-1)/
	cp $(man-api) $(man-dir-3)/
	cp $(library) $(library-dir)/

clean:
	$(RM) $(exe)
