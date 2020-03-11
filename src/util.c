#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void close_void(int fd)
{
	int errnum = errno;
	close(fd);
	errno = errnum;
}

char *dll_name_to_path(const char *name)
{
	if (*name != '-' && strchr(name, '/')) {
		return str_dup(name);
	} else {
		return str_cat("./", name);
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

void *end_of(const void *mem, size_t mem_size, size_t end_size)
{
	return (char *)mem + mem_size - end_size;
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

int one_time_pipe(int fds[2])
{
	char path[L_tmpnam];
	fds[0] = fds[1] = -1;
	for (long i = 0; i < TMP_MAX && tmpnam(path); ++i) {
		if ((fds[0] = open(path, O_RDONLY|O_CREAT|O_EXCL, 0600)) >= 0) {
			if ((fds[1] = open(path, O_WRONLY)) < 0)
				goto error_write;
			if (unlink(path) < 0) goto error_unlink;
			return 0;
		} else if (errno != EEXIST) {
			goto error_read;
		}
	}
error_unlink:
	close_void(fds[1]);
error_write:
	close_void(fds[0]);
error_read:
	// Fall back to a pipe that is non-blocking to prevent hanging.
	if (pipe(fds)) return -1;
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
	fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL) | O_NONBLOCK);
	return 0;
}

int prefix_lines(const char *prefix, int fd, FILE *out)
{
	size_t prefix_len = strlen(prefix);
	bool line_begun = true;
	char buf[BUFSIZ];
	ssize_t n_read = sizeof(buf);
	char *head = buf + n_read;
	do {
		if (head >= buf + n_read) {
			errno = 0;
			n_read = read_nointr(fd, buf, sizeof(buf));
			if (n_read <= 0) break;
			head = buf;
		}
		if (line_begun) {
			fwrite(prefix, 1, prefix_len, out);
		}
		size_t n_left = n_read + buf - head;
		char *nl = memchr(head, '\n', n_left);
		line_begun = nl != NULL;
		size_t line_len = line_begun ? (size_t)(nl - head + 1) : n_left;
		fwrite(head, 1, line_len, out);
		head = head + line_len;
	} while (head < buf + n_read || n_read == sizeof(buf));
	if (!line_begun) {
		fputc('\n', out);
	}
	return 0;
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

char *str_dup(const char *str)
{
	size_t size = strlen(str) + 1;
	return memcpy(xmalloc(size), str, size);
}

char *str_cat(const char *str1, const char *str2)
{
	size_t len1 = strlen(str1);
	size_t size2 = strlen(str2) + 1;
	char *cat = xmalloc(len1 + size2);
	memcpy(cat, str1, len1);
	memcpy(cat + len1, str2, size2);
	return cat;
}

void system_error(const char *prog_name)
{
	fprintf(stderr, "%s: ", prog_name);
	perror("System error");
	exit(EXIT_FAILURE);
}
