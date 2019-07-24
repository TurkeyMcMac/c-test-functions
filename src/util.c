#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *dll_name_to_path(const char *name)
{
	size_t size = strlen(name) + 1;
	if (*name == '/') {
		return memcpy(xmalloc(size), name, size);
	} else {
		char *path = xmalloc(size + 2);
		memcpy(path, "./", 2);
		memcpy(path + 2, name, size);
		return path;
	}
}

int dup2_nointr(int orig, int new)
{
	int ret;
	do {
		ret = dup2(orig, new);
	} while (ret < 0 && errno == EINTR);
	return ret;
}

void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
	size_t append, size_t item_size)
{
	size_t old_len = *len;
	*len += append;
	if (*len > *cap) {
		*cap = *len * 3 / 2;
		*list = xrealloc(*list, *cap * item_size);
	}
	return (char *)*list + old_len * item_size;
}

pid_t safe_fork(void)
{
	if (fflush(NULL)) return -1;
	return fork();
}

void system_error(const char *prog_name)
{
	fprintf(stderr, "%s: ", prog_name);
	perror("System error");
	exit(EXIT_FAILURE);
}

pid_t wait_nointr(int *exit_info)
{
	pid_t pid;
	do {
		pid = wait(exit_info);
	} while (pid < 0 && errno == EINTR);
	return pid;
}
