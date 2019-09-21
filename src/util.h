#ifndef UTIL_H_
#define UTIL_H_

#include <libctf.h>
#include <sys/wait.h>
#include <unistd.h>

/* Evaluate the argument and convert it to a string. */
#define STRINGIFY(thing) STRINGIFY_(thing)
#define STRINGIFY_(thing) #thing

/* the string version of the test symbol prefix and its length. */
#define PREFIX STRINGIFY(CTF_PREFIX)
#define PREFIX_SIZE 5
#define SHIFT_KEEP (PREFIX_SIZE - 1)

/* the string version of the test symbol suffix and its length. */
#define SUFFIX STRINGIFY(CTF_SUFFIX)
#define SUFFIX_SIZE 5

/* Return a functional copy of the executable name `name` containing at least 1
 * slash. */
char *dll_name_to_path(const char *name);

/* Like dup2, but repeats until it is uninterrupted. */
int dup2_nointr(int orig, int new);

/* Find `part` inside `within`, returning the starting address that matches or
 * NULL if not found. */
void *find(const void *within, size_t within_size,
	const void *part, size_t part_size);

void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
	size_t append, size_t item_size);

/* Grow `list` by `append` members. `cap` is the number allocated and `len` is
 * the number used. No arguments are references. */
#define GROW(list, len, cap, append) \
	grow_((void **)&(list), &(len), &(cap), append, sizeof *(list))

ssize_t read_nointr(int fd, void *buf, size_t count);

/* Fork after flushing all buffers to avoid duplicated output. */
pid_t safe_fork(void);

/* Allocate a duplicate string. */
char *str_dup(const char *str);

/* Allocate a string that is str1 followed by str2 with no NULL between. */
char *str_cat(const char *str1, const char *str2);

/* Print a system error from `errno` with the program name `prog_name`. */
void system_error(const char *prog_name);

/* If a system doesn't have WCOREDUMP, assume the core was not dumped. */
#ifndef WCOREDUMP
#	define WCOREDUMP(status) ((status) & 0)
#endif

#endif /* UTIL_H_ */
