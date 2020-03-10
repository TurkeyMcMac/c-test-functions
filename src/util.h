#ifndef UTIL_H_
#define UTIL_H_

#include <libctf.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/* Just like close(fd), but returns void and does not alter errno. */
void close_void(int fd);

/* Return a functional copy of the executable name `name` containing at least 1
 * slash and having a first character other than '-'. */
char *dll_name_to_path(const char *name);

/* Like dup2, but repeats until it is uninterrupted. */
int dup2_nointr(int orig, int new);

/* Get a pointer to the end with size `end_size` of `mem` sized `mem_size`.
 * Precondition: `end_size` <= `mem_size` */
void *end_of(const void *mem, size_t mem_size, size_t end_size);

/* Find `part` inside `within`, returning the starting address that matches or
 * NULL if not found. */
void *find(const void *within, size_t within_size,
	const void *part, size_t part_size);

/* Grow `list` by `append` members. `cap` is the number allocated and `len` is
 * the number used. No arguments are references. */
#define GROW(list, len, cap, append) \
	grow_((void **)&(list), &(len), &(cap), append, sizeof *(list))

void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
	size_t append, size_t item_size);

/* Create a pipe into which the writer will first write all data then close the
 * write end. After that, all data will be read out the read end. Regular pipes
 * are not suited for this because they block if too much data is written in
 * before being read out. A non-blocking but otherwise regular pipe is returned
 * if a temporary file could not be created. */
int one_time_pipe(int fds[2]);

/* the string version of the test symbol prefix and its length. */
#define PREFIX STRINGIFY(CTF_PREFIX)
#define PREFIX_SIZE 5

/* Read all the lines from fd, give them the prefix, and write them to out. 0
 * means success. A negative return value means failure with errno set. */
int prefix_lines(const char *prefix, int fd, FILE *out);

/* Just like read(fd, buf, count), but retries after interrupts. */
ssize_t read_nointr(int fd, void *buf, size_t count);

/* Fork after flushing all buffers to avoid duplicated output. */
pid_t safe_fork(void);

/* Allocate a duplicate string. */
char *str_dup(const char *str);

/* Allocate a string that is str1 followed by str2 with no NULL between. */
char *str_cat(const char *str1, const char *str2);

/* Evaluate the argument and convert it to a string. */
#define STRINGIFY(thing) STRINGIFY_(thing)
#define STRINGIFY_(thing) #thing

/* the string version of the test symbol suffix and its length. */
#define SUFFIX STRINGIFY(CTF_SUFFIX)
#define SUFFIX_SIZE 5

/* Print a system error from `errno` with the program name `prog_name`. */
void system_error(const char *prog_name);

/* If a system doesn't have WCOREDUMP, assume the core was not dumped. */
#ifndef WCOREDUMP
#	define WCOREDUMP(status) ((status) & 0)
#endif

#endif /* UTIL_H_ */
