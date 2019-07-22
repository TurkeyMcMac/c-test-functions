exe = ctf
c-flags = $(CFLAGS) -Ilib

$(exe): src lib
	$(CC) $(c-flags) -o $(exe) src/*.c
