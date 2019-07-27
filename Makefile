exe = ceeteef
c-flags = -Ilib -std=c99 -Wall -Wextra -Wpedantic $(CFLAGS)

all: $(exe)

$(exe): src lib
	$(CC) $(c-flags) -o $(exe) src/*.c

clean:
	$(RM) $(exe)
