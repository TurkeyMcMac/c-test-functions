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

int run_tests(struct test *tests, size_t n_tests)
{
	for (size_t i = 0; i < n_tests; ++i) {
		int pipefds[2];
		pipe(pipefds);
		tests[i].read_fd = pipefds[0];
		if (start_test(tests[i].fun, &tests[i].pid, pipefds[1])) {
			while (i--) {
				close(tests[i].read_fd);
			}
			return -1;
		}
	}
	size_t n_left = n_tests;
	int exit_info;
	pid_t pid;
	while (n_left > 0 && (pid = wait_nointr(&exit_info)) >= 0) {
		struct test *test = NULL;
		for (size_t i = 0; i < n_tests; ++i) {
			if (tests[i].pid == pid) {
				test = &tests[i];
				--n_left;
				break;
			}
		}
		if (!test) continue;
		if (fcntl(test->read_fd, F_SETFL, O_NONBLOCK)) return -1;
		FILE *out = fdopen(test->read_fd, "r");
		if (!out) return -1;
		char *line = NULL;
		size_t line_cap = 0;
		ssize_t len;
		printf("-- %s --\n", test->name);
		while ((len = getline(&line, &line_cap, out)) > 0)
		{
			if (line[len - 1] != '\n') {
				line[len] = '\n';
				++len;
			}
			printf("%s:%.*s", test->name, (int)len, line);
		}
		fclose(out);
		int exit_code = WEXITSTATUS(exit_info);
		int core_dump = WCOREDUMP(exit_info);
		printf("%s %s   Exit code: %d%s\n",
			test->name,
			exit_code == 0 && !core_dump ?
				"SUCCEEDED" : "FAILED",
			exit_code,
			core_dump ? "   Core dumped" : "");
	}
	if (errno != ECHILD) return -1;
	return 0;
}
