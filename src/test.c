#include "test.h"
#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int write_test(struct test *test)
{
	int errnum; // For swapping out errno
	test->pid = -1;
	if (fcntl(test->read_fd, F_SETFL, O_NONBLOCK)) return -1;
	FILE *out = fdopen(test->read_fd, "r");
	if (!out) return -1;
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
	if (errno && errno != EWOULDBLOCK && errno != EINTR) return -1;
	return 0;
}

static volatile sig_atomic_t n_alarms;

static void receive_alarm(int signum)
{
	(void)signum;
	++n_alarms;
}

int run_tests(struct test *tests, size_t n_tests, int timeout)
{
	int retval = 0;
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
	struct sigaction old_action;
	if (timeout > 0) {
		n_alarms = 0;
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		action.sa_handler = receive_alarm;
		sigaction(SIGALRM, &action, &old_action);
		alarm(timeout);
	}
	sigset_t set, old_set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	while (n_left > 0 && (errno = 0, pid = wait(&exit_info)) >= 0) {
		struct test *test = NULL;
		for (size_t i = 0; i < n_tests; ++i) {
			if (tests[i].pid == pid) {
				test = &tests[i];
				--n_left;
				break;
			}
		}
		if (!test) continue;
		sigprocmask(SIG_BLOCK, &set, &old_set);
		write_test(test);
		print_exit_info(test, exit_info);
		if (n_alarms > 0) goto timed_out;
		sigprocmask(SIG_SETMASK, &old_set, NULL);
	}
	switch (errno) {
	case 0:
	case ECHILD:
		break;
	case EINTR:
		if (n_alarms > 0) goto timed_out;
		/* FALLTHROUGH */
	default:
		goto error;
	}
	goto remove_alarm;

timed_out:
	for (size_t i = 0; i < n_tests; ++i) {
		struct test *test = &tests[i];
		if (test->read_fd >= 0 && test->pid >= 0) {
			kill(test->pid, SIGTERM);
			write_test(test);
			printf("%s FAILED   Timed out in %ds\n",
				test->name, n_alarms * timeout);
		}
	}
remove_alarm:
	if (timeout > 0) {
		alarm(0);
		sigaction(SIGALRM, &old_action, NULL);
	}
	return retval;

error:
	for (size_t i = 0; i < n_tests; ++i) {
		if (tests[i].read_fd >= 0) close(tests[i].read_fd);
		if (tests[i].pid >= 0) kill(tests[i].pid, SIGTERM);
	}
	if (timeout > 0) sigaction(SIGALRM, &old_action, NULL);
	retval = -1;
	goto remove_alarm;
}
