#ifndef UTIL_H_
#define UTIL_H_

#include <libctf.h>
#include <unistd.h>

#define STRINGIFY(thing) STRINGIFY_(thing)
#define STRINGIFY_(thing) #thing

#define PREFIX STRINGIFY(CTF_PREFIX)
#define PREFIX_SIZE 11
#define SHIFT_KEEP (PREFIX_SIZE - 1)

#define SUFFIX STRINGIFY(CTF_SUFFIX)
#define SUFFIX_SIZE 11

char *dll_name_to_path(const char *name);

int dup2_nointr(int orig, int new);

void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
	size_t append, size_t item_size);

#define GROW(list, len, cap, append) \
	grow_((void **)&(list), &(len), &(cap), append, sizeof *(list))

pid_t safe_fork(void);

void system_error(const char *prog_name);

pid_t wait_nointr(int *exit_info);

#endif /* UTIL_H_ */
