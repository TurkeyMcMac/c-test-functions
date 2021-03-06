#include "test.h"
#include "style.h"
#include "util.h"
#include "xalloc.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/time.h>

size_t get_max_tests(void)
{
	// I'm just guessing with the formula here.
	struct rlimit fd_lim = { .rlim_cur = 0 };
	getrlimit(RLIMIT_NOFILE, &fd_lim);
	return fd_lim.rlim_cur > 8 ? fd_lim.rlim_cur - 8 : 1;
}

static void print_failed(const char *name)
{
	printf("%s%s %sFAILED%s   ",
		style_bold(), name, style_fg_red(), style_end_all());
}

static void print_succeeded(const char *name)
{
	printf("%s%s %sSUCCEEDED%s\n",
		style_bold(), name, style_fg_green(), style_end_all());
}

static int start_test(struct test *tests, size_t idx, int out_fd)
{
	pid_t pid;
	if ((pid = safe_fork())) {
		if (pid < 0) return -1;
		close_void(out_fd);
		tests[idx].pid = pid;
		return 0;
	} else {
		if (dup2_nointr(out_fd, STDOUT_FILENO) < 0
		 || dup2_nointr(out_fd, STDERR_FILENO) < 0)
			exit(EXIT_FAILURE);
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		close(out_fd);
		// Don't deprive the test of available file descriptors:
		for (size_t j = 0; j <= idx; ++j) {
			close(tests[j].read_fd);
		}
		exit(tests[idx].fun());
	}
	return -1;
}

static void print_exit_info(const struct test *test, int exit_info)
{
	if (WIFEXITED(exit_info) && WEXITSTATUS(exit_info)) {
		print_failed(test->name);
		printf("Exited with status %d\n", WEXITSTATUS(exit_info));
	} else if (WIFSIGNALED(exit_info)) {
		print_failed(test->name);
		printf("Terminated by signal %d%s\n", WTERMSIG(exit_info),
			WCOREDUMP(exit_info) ? "   Core dumped" : "");
	} else {
		print_succeeded(test->name);
	}
}

static int write_test(struct test *test)
{
	int fd = test->read_fd;
	char *prefix = str_cat(test->name, ":");
	test->pid = -1;
	printf("-- %s%s%s --\n", style_bold(), test->name, style_end_all());
	if (prefix_lines(prefix, fd, stdout)) goto error;
	free(prefix);
	test->read_fd = -1;
	close_void(fd);
	return 0;

error:
	free(prefix);
	return -1;
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
		if (one_time_pipe(pipefds)) {
			n_tests = i;
			goto error;
		}
		tests[i].read_fd = pipefds[0];
		if (start_test(tests, i, pipefds[1])) {
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

error:
	for (size_t i = 0; i < n_tests; ++i) {
		if (tests[i].pid >= 0) kill(tests[i].pid, SIGTERM);
	}
	if (timeout > 0) sigaction(SIGALRM, &old_action, NULL);
	retval = -1;
	goto remove_alarm;

timed_out:
	for (size_t i = 0; i < n_tests; ++i) {
		struct test *test = &tests[i];
		if (test->read_fd >= 0 && test->pid >= 0) {
			kill(test->pid, SIGTERM);
			write_test(test);
			print_failed(test->name);
			printf("Timed out in %ds\n", n_alarms * timeout);
		}
	}
remove_alarm:
	if (timeout > 0) {
		alarm(0);
		sigaction(SIGALRM, &old_action, NULL);
	}
	for (size_t i = 0; i < n_tests; ++i) {
		if (tests[i].read_fd >= 0) close_void(tests[i].read_fd);
	}
	return retval;
}
