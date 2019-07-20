#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PREFIX "ctf_test___"
#define PREFIX_SIZE 11
#define SHIFT_KEEP (PREFIX_SIZE - 1)

static void *xmalloc(size_t size)
{
	void *mem = malloc(size ? size : 1);
	if (!mem) abort();
	return mem;
}

static void *xrealloc(void *mem, size_t size)
{
	void *new_mem = realloc(mem, size ? size : 1);
	if (!new_mem) abort();
	return new_mem;
}

static char *path_to_arg(const char *path)
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

static int start_nm_proc(const char *fpath, pid_t *pidp, int *fdp)
{
	int pipefds[2];
	pid_t pid;
	char *arg1 = path_to_arg(fpath);
	if (pipe(pipefds)) goto error_pipe;
	if ((pid = fork())) {
		free(arg1);
		if (pid < 0) goto error_fork;
		if (close(pipefds[1])) goto error_close;
		*pidp = pid;
		*fdp = pipefds[0];
		return 0;
	} else {
		char arg0[] = "nm";
		char *argv[] = {arg0, arg1, NULL};
		errno = 0;
		while (dup2(pipefds[1], STDOUT_FILENO) < 0 && errno == EINTR)
			;
		if (errno) exit(EXIT_FAILURE);
		if (close(pipefds[0]) || close(pipefds[1])) exit(EXIT_FAILURE);
		execvp("nm", argv);
	}

error_pipe:
	free(arg1);
error_close:
error_fork:
	return -1;
}

static void *grow_(void **list, size_t *restrict len, size_t *restrict cap,
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

#define GROW(list, len, cap, append) \
	grow_((void **)&(list), &(len), &(cap), append, sizeof *(list))

static int scan_test_names(int in_fd, char ***names, size_t *n_names,
	size_t *names_cap)
{
	const char prefix[PREFIX_SIZE] = PREFIX;
	char buf_stored[BUFSIZ + SHIFT_KEEP];
	size_t buf_space = sizeof(buf_stored);
	char *buf = buf_stored;
	char __[10000];
	char *wip_sym = NULL;
	size_t wip_sym_len, wip_sym_cap;
	bool has_more;
	do {
		errno = 0;
		size_t n_read = read(in_fd, buf, buf_space);
		has_more = n_read == buf_space;
		if (errno) return -1;
		char *search = buf_stored;
		size_t n_search = n_read + SHIFT_KEEP;
		buf_space = sizeof(buf_stored) - SHIFT_KEEP;
		if (wip_sym) {
				search = buf;
				n_search -= SHIFT_KEEP;
			char *end = memchr(search, '\n', n_search);
			if (end) {
				size_t n_added = end - search;
				char *more = GROW(wip_sym, wip_sym_len,
					wip_sym_cap, n_added + 1);
				memcpy(more, search, n_added);
				more[n_added] = '\0';
				char **push = GROW(*names, *n_names, *names_cap,
					1);
				*push = wip_sym;
				wip_sym = NULL;
				search += n_added + 1;
				n_search -= n_added + 1;
			} else {
				char *more = GROW(wip_sym, wip_sym_len,
					wip_sym_cap, n_search);
				memcpy(more, search, n_search);
				n_search = 0;
				buf_space = sizeof(buf_stored);
			}
		}
		while (n_search > 0) {
			char *sym = memmem(search, n_search, prefix,
				PREFIX_SIZE);
			if (sym) {
				size_t search_left = n_search + search - sym;
				char *sym_end = memchr(sym + PREFIX_SIZE, '\n',
					search_left - PREFIX_SIZE);
				if (sym_end) {
					size_t sym_len = sym_end - sym;
					char **push = GROW(*names, *n_names,
						*names_cap, 1);
					char *saved = xmalloc(sym_len + 1);
					memcpy(saved, sym, sym_len);
					saved[sym_len] = '\0';
					*push = saved;
					char *old_search = search;
					search = sym + sym_len + 1;
					n_search -= search - old_search;
				} else {
					wip_sym_len = wip_sym_cap = search_left;
					wip_sym = xmalloc(wip_sym_cap);
					memcpy(wip_sym, sym, wip_sym_len);
					break;
				}
			} else {
				break;
			}
		}
		size_t shift_keep = sizeof(buf_stored) - buf_space;
		memmove(buf_stored, buf_stored + buf_space, shift_keep);
		buf = buf_stored + shift_keep;
	} while (has_more);
	return 0;
}

int main(int argc, char *argv[])
{
	size_t n_names = 0, names_cap = 5;
	char **names = xmalloc(names_cap * sizeof(*names));
	pid_t pid;
	int fd;
	if (start_nm_proc(argv[1], &pid, &fd)) exit(EXIT_FAILURE);
	if (scan_test_names(fd, &names, &n_names, &names_cap))
		exit(EXIT_FAILURE);
	printf("tests:\n");
	for (size_t i = 0; i < n_names; ++i) {
		printf(" - %s\n", names[i]);
	}
}

void ctf_test___111111(void)
{}
void ctf_test___222222(void)
{}