#ifndef UTIL_H_
#define UTIL_H_

#include <libctf.h>
#include <unistd.h>

/* Evaluate the argument and convert it to a string. */
#define STRINGIFY(thing) STRINGIFY_(thing)
#define STRINGIFY_(thing) #thing

/* the string version of the test symbol prefix and its length. */
#define PREFIX STRINGIFY(CTF_PREFIX)
#define PREFIX_SIZE 11
#define SHIFT_KEEP (PREFIX_SIZE - 1)

/* the string version of the test symbol suffix and its length. */
#define SUFFIX STRINGIFY(CTF_SUFFIX)
#define SUFFIX_SIZE 11

/* Return a functional copy of the executable name `name` containing at least 1
 * slash. */
char *dll_name_to_path(const char *name);

/* Like dup2, but repeats until it is uninterrupted. */
int dup2_nointr(int orig, int new);

void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
	size_t append, size_t item_size);

/* Grow `list` by `append` members. `cap` is the number allocated and `len` is
 * the number used. No arguments are references. */
#define GROW(list, len, cap, append) \
	grow_((void **)&(list), &(len), &(cap), append, sizeof *(list))

/* Fork after flushing all buffers to avoid duplicated output. */
pid_t safe_fork(void);

/* Print a system error from `errno` with the program name `prog_name`. */
void system_error(const char *prog_name);

#endif /* UTIL_H_ */
