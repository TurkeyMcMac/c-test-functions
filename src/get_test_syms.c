#include "get_test_syms.h"
#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int start_nm_proc(const char *fpath, pid_t *pidp, int *fdp)
{
	int pipefds[2];
	pid_t pid;
	char *arg1 = dll_name_to_path(fpath);
	if (pipe(pipefds)) {
		free(arg1);
		return -1;
	}
	if ((pid = safe_fork())) {
		free(arg1);
		if (pid < 0) return -1;
		close(pipefds[1]);
		*pidp = pid;
		*fdp = pipefds[0];
		return 0;
	} else {
		char arg0[] = "nm";
		char *argv[] = {arg0, arg1, NULL};
		if (dup2_nointr(pipefds[1], STDOUT_FILENO) < 0)
			exit(EXIT_FAILURE);
		// Redirect stderr to silence it. Probably symbols won't show up
		// within it, so I think it'll be fine.
		if (dup2_nointr(pipefds[1], STDERR_FILENO) < 0)
			exit(EXIT_FAILURE);
		close(pipefds[0]);
		close(pipefds[1]);
		execvp("nm", argv);
	}
	return 0;
}

static void confirm_name(char ***names, size_t *n_names, size_t *names_cap,
	char *name, size_t name_len)
{
	if (memmem(name, name_len + 1, SUFFIX, SUFFIX_SIZE + 1)) {
		char **push = GROW(*names, *n_names, *names_cap, 1);
		*push = name;
	} else {
		free(name);
	}
}

static int scan_test_names(int in_fd, char ***names, size_t *n_names,
	size_t *names_cap)
{
	static const char prefix[PREFIX_SIZE] = PREFIX;
	char buf_stored[BUFSIZ + SHIFT_KEEP];
	size_t buf_space = sizeof(buf_stored);
	char *buf = buf_stored;
	char *wip_sym = NULL;
	size_t wip_sym_len, wip_sym_cap;
	bool has_more;
	do {
		errno = 0;
		size_t n_read = read(in_fd, buf, buf_space);
		has_more = n_read == buf_space;
		if (errno) return -1;
		char *search = buf_stored;
		size_t n_search = n_read + buf - buf_stored;
		buf_space = sizeof(buf_stored) - SHIFT_KEEP;
		if (wip_sym) {
			search = buf;
			n_search -= buf - buf_stored;
			char *end = memchr(search, '\n', n_search);
			if (end) {
				size_t n_added = end - search;
				char *more = GROW(wip_sym, wip_sym_len,
					wip_sym_cap, n_added + 1);
				memcpy(more, search, n_added);
				more[n_added] = '\0';
				confirm_name(names, n_names, names_cap,
					wip_sym, wip_sym_len);
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
					char *saved = xmalloc(sym_len + 1);
					memcpy(saved, sym, sym_len);
					saved[sym_len] = '\0';
					confirm_name(names, n_names, names_cap,
						saved, sym_len);
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

int get_test_syms(const char *path, char ***names, size_t *n_names)
{
	pid_t pid;
	int fd;
	size_t names_cap = 0;
	*names = NULL;
	*n_names = 0;
	if (start_nm_proc(path, &pid, &fd)) return -1;
	if (scan_test_names(fd, names, n_names, &names_cap)) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
