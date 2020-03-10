#include "get_test_syms.h"
#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// The number of bytes kept between fillings of the buffer
#define SHIFT_KEEP (PREFIX_SIZE - 1)

static int start_nm_proc(const char *fpath, pid_t *pidp, int *fdp, int *errfdp)
{
	int pipefds[2];
	int nm_read, nm_write, err_read, err_write;
	pid_t pid;
	if (pipe(pipefds)) goto error_pipe_nm;
	nm_read = pipefds[0];
	nm_write = pipefds[1];
	if (one_time_pipe(pipefds)) goto error_pipe_err;
	err_read = pipefds[0];
	err_write = pipefds[1];
	if ((pid = safe_fork())) {
		if (pid < 0) goto error_fork;
		close_void(nm_write);
		close_void(err_write);
		*pidp = pid;
		*fdp = nm_read;
		*errfdp = err_read;
		return 0;
	} else {
		char arg0[] = "nm";
		char arg1[] = "-P";
		char *arg2 = dll_name_to_path(fpath);
		char *argv[] = {arg0, arg1, arg2, NULL};
		if (dup2_nointr(nm_write, STDOUT_FILENO) < 0) goto child_error;
		if (dup2_nointr(err_write, STDERR_FILENO) < 0) goto child_error;
		// Not closing the read ends here disables SIGPIPE.
		close(nm_write);
		close(err_write);
		execvp("nm", argv);
child_error:
		perror("Failed to run \"nm\" command");
		exit(EXIT_FAILURE);
	}

error_fork:
	close_void(err_read);
	close_void(err_write);
error_pipe_err:
	close_void(nm_read);
	close_void(nm_write);
error_pipe_nm:
	*errfdp = -1; // An invalid but harmless fd to try and close.
	return -1;
}

static void confirm_name(char ***names, size_t *n_names, size_t *names_cap,
	char *name, size_t name_len)
{
	if (!memcmp(end_of(name, name_len, SUFFIX_SIZE), SUFFIX, SUFFIX_SIZE)) {
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
			char *end = memchr(search, ' ', n_search);
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
			char *sym = find(search, n_search, prefix,
				PREFIX_SIZE);
			if (sym) {
				size_t search_left = n_search + search - sym;
				char *sym_end = memchr(sym + PREFIX_SIZE, ' ',
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

// If there was an error with a system call, errinfo is set to -errno. If the
// process exited nonzero, errinfo is set to the fd of the process stderr.
int get_test_syms(const char *path, char ***names, size_t *n_names,
	size_t *names_cap, int *errinfo)
{
	int exit_info;
	pid_t pid;
	int fd;
	if (start_nm_proc(path, &pid, &fd, errinfo)) goto error;
	int scan_err = scan_test_names(fd, names, n_names, names_cap);
	close_void(fd);
	if (scan_err) goto error;
	int errnum = errno;
	if (waitpid(pid, &exit_info, 0) == pid) {
		if ((WIFEXITED(exit_info) && WEXITSTATUS(exit_info))
		 || WIFSIGNALED(exit_info))
			// errinfo is set to the stderr fd.
			return -1;
	} else {
		// Ignore the error
		errno = errnum;
	}
	close_void(*errinfo);
	return 0;

error:
	close_void(*errinfo);
	*errinfo = -errno;
	return -1;
}

void print_test_syms_error(const char *prog_name, int errinfo)
{
	if (errinfo < 0) {
		errno = -errinfo;
		system_error(prog_name);
	} else {
		int err_read_fd = errinfo;
		prefix_lines(str_cat(prog_name, ": "), err_read_fd, stderr);
		exit(EXIT_FAILURE);
	}
}
