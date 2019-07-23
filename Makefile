exe = ctf
c-flags = $(CFLAGS) -Ilib

all: $(exe)

$(exe): src lib
	$(CC) $(c-flags) -o $(exe) src/*.c

clean:
	$(RM) $(exe)
