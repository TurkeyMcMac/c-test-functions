#include "test.h"
#include "util.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

FILE *start_test(test_fun fun, pid_t *test_pid)
{
	pid_t pid;
	int pipefds[2];
	if (pipe(pipefds)) return NULL;
	if ((pid = safe_fork())) {
		if (pid < 0) return NULL;
		close(pipefds[1]);
		FILE *out = fdopen(pipefds[0], "r");
		if (!out) kill(pid, SIGKILL);
		*test_pid = pid;
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
		exit(fun());
	}
	return NULL;
}
