#include "test.h"
#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static int start_test(test_fun fun, pid_t *test_pid, int out_fd)
{
	pid_t pid;
	if ((pid = safe_fork())) {
		if (pid < 0) return -1;
		close(out_fd);
		*test_pid = pid;
		return 0;
	} else {
		if (dup2_nointr(out_fd, STDOUT_FILENO) < 0
		 || dup2_nointr(out_fd, STDERR_FILENO) < 0)
			exit(EXIT_FAILURE);
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		close(out_fd);
		exit(fun());
	}
	return -1;
}

static void print_exit_info(const struct test *test, int exit_info)
{
	int exit_code = WEXITSTATUS(exit_info);
	int core_dump = WCOREDUMP(exit_info);
	if (WIFEXITED(exit_info) && WEXITSTATUS(exit_info)) {
		printf("%s FAILED   Exited with status %d\n",
			test->name, WEXITSTATUS(exit_info));
	} else if (WIFSIGNALED(exit_info)) {
		printf("%s FAILED   Terminated by signal %d%s\n",
			test->name, WTERMSIG(exit_info),
			WCOREDUMP(exit_info) ? "   Core dumped" : "");
	} else {
		printf("%s SUCCEEDED\n", test->name);
	}
}

int run_tests(struct test *tests, size_t n_tests)
{
	int errnum; // For swapping out errno
	for (size_t i = 0; i < n_tests; ++i) {
		int pipefds[2];
		if (pipe(pipefds)) {
			n_tests = i;
			goto error;
		}
		tests[i].read_fd = pipefds[0];
		if (start_test(tests[i].fun, &tests[i].pid, pipefds[1])) {
			tests[i].pid = -1;
			n_tests = i + 1;
			goto error;
		}
	}
	size_t n_left = n_tests;
	int exit_info;
	pid_t pid;
	while (n_left > 0 && (errno = 0, pid = wait_nointr(&exit_info)) >= 0) {
		struct test *test = NULL;
		for (size_t i = 0; i < n_tests; ++i) {
			if (tests[i].pid == pid) {
				test = &tests[i];
				--n_left;
				break;
			}
		}
		if (!test) continue;
		test->pid = -1;
		if (fcntl(test->read_fd, F_SETFL, O_NONBLOCK)) goto error;
		FILE *out = fdopen(test->read_fd, "r");
		if (!out) goto error;
		char *line = NULL;
		size_t line_cap = 0;
		ssize_t len;
		printf("-- %s --\n", test->name);
		while ((errno = 0, len = getline(&line, &line_cap, out)) > 0)
		{
			if (line[len - 1] != '\n') {
				line[len] = '\n';
				++len;
			}
			printf("%s:%.*s", test->name, (int)len, line);
		}
		errnum = errno;
		fclose(out);
		errno = errnum;
		test->read_fd = -1;
		if (errno && errno != EWOULDBLOCK) goto error;
		print_exit_info(test, exit_info);
	}
	if (errno && errno != ECHILD) goto error;
	return 0;

error:
	for (size_t i = 0; i < n_tests; ++i) {
		if (tests[i].read_fd >= 0) close(tests[i].read_fd);
		if (tests[i].pid >= 0) kill(tests[i].pid, SIGTERM);
	}
	return -1;
}
