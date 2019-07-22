#include "util.h"
#include "xalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char *path_to_arg(const char *path)
{
	size_t path_len = strlen(path);
	char *arg = xmalloc(path_len + 3);
	if (*path == '-') {
		memcpy(arg, "./", 2);
		memcpy(arg + 2, path, path_len + 1);
	} else {
		memcpy(arg, path, path_len + 1);
	}
	return arg;
}

pid_t safe_fork(void)
{
	if (fflush(NULL)) return -1;
	return fork();
}

void system_error(const char *prog_name)
{
	fprintf(stderr, "%s", prog_name);
	perror(": System error: ");
	exit(EXIT_FAILURE);
}
