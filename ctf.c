#include "libctf.h"
#include <dlfcn.h>
#include <errno.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STRINGIFY(thing) STRINGIFY_(thing)
#define STRINGIFY_(thing) #thing

#define PREFIX STRINGIFY(CTF_PREFIX)
#define PREFIX_SIZE 11
#define SHIFT_KEEP (PREFIX_SIZE - 1)

#define SUFFIX STRINGIFY(CTF_SUFFIX)
#define SUFFIX_SIZE 11

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
	if (fflush(NULL)) return -1;
	char *arg1 = path_to_arg(fpath);
	if (pipe(pipefds)) {
		free(arg1);
		return -1;
	}
	if ((pid = fork())) {
		free(arg1);
		if (pid < 0) return -1;
		close(pipefds[1]);
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
		close(pipefds[0]);
		close(pipefds[1]);
		execvp("nm", argv);
	}
	return 0;
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

void system_error(const char *prog_name)
{
	fprintf(stderr, "%s", prog_name);
	perror(": System error: ");
	exit(EXIT_FAILURE);
}

typedef void (*test_fun)(void);

static FILE *start_test(test_fun fun)
{
	pid_t pid;
	int pipefds[2];
	if (fflush(NULL)) return NULL;
	if (pipe(pipefds)) return NULL;
	if ((pid = fork())) {
		if (pid < 0) return NULL;
		close(pipefds[1]);
		FILE *out = fdopen(pipefds[0], "r");
		if (!out) kill(pid, SIGKILL);
		return out;
	} else {
		errno = 0;
		while (dup2(pipefds[1], STDOUT_FILENO) < 0 && errno == EINTR)
			;
		if (errno) return NULL;
		while (dup2(pipefds[1], STDERR_FILENO) < 0 && errno == EINTR)
			;
		close(pipefds[0]);
		close(pipefds[1]);
		fun();
		exit(0);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int err;
	size_t n_names = 0, names_cap = 5;
	char **names = xmalloc(names_cap * sizeof(*names));
	pid_t pid;
	int fd;
	regex_t name_pat;
	err = regcomp(&name_pat, argv[1], REG_NOSUB);
	if (err) {
		char err_buf[256];
		regerror(err, &name_pat, err_buf, sizeof(err_buf));
		fprintf(stderr, "%s: Regular expression error: %s\n",
			argv[0], err_buf);
		exit(EXIT_FAILURE);
	}
	if (start_nm_proc(argv[2], &pid, &fd)) system_error(argv[0]);
	if (scan_test_names(fd, &names, &n_names, &names_cap))
		system_error(argv[0]);
	void *dl = dlopen(argv[2], RTLD_LAZY);
	for (size_t i = 0; i < n_names; ++i) {
		void *sym = dlsym(dl, names[i]);
		if (sym) {
			char *test_name = names[i] + PREFIX_SIZE;
			*strstr(test_name, SUFFIX) = '\0';
			if (!regexec(&name_pat, test_name, 0, NULL, 0)) {
				test_fun fun = *(test_fun *)&sym;
				FILE *o = start_test(fun);
				if (!o) system_error(argv[0]);
				char *line = NULL;
				size_t line_cap = 0;
				ssize_t len;
				printf("-- %s --\n", test_name);
				while ((len = getline(&line, &line_cap, o)) > 0)
				{
					if (line[len - 1] != '\n') {
						line[len] = '\n';
						++len;
					}
					printf("%s:%.*s",
						test_name, (int)len, line);
				}
			}
		}
	}
	dlclose(dl);
	close(fd);
	regfree(&name_pat);
}

CTF_TEST(test_1,
	printf("test 1\n");
)

CTF_TEST(test_2,
	printf("test 2\n");
)
