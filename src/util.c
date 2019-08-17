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

void *find(const void *within, size_t within_size,
	const void *part, size_t part_size)
{
	if (within_size >= part_size) {
		for (size_t i = 0; i <= within_size - part_size; ++i) {
			void *chunk = (char *)within + i;
			if (!memcmp(part, chunk, part_size)) return chunk;
		}
	}
	return NULL;
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

ssize_t read_nointr(int fd, void *buf, size_t count)
{
	ssize_t n_read;
	while ((n_read = read(fd, buf, count)) < 0 && errno == EINTR)
		;
	return n_read;
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
