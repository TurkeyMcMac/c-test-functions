#ifndef TEST_H_
#define TEST_H_

#include <stddef.h>
#include <unistd.h>

/* A pointer to a test function. */
typedef int (*test_fun)(void);

/* Per-test information. */
struct test {
	/* The test name, excluding the circumfix. */
	const char *name;
	/* The test function. */
	test_fun fun;
	/* The PID of the test runner. Internal. */
	pid_t pid;
	/* Read output from here. Internal. */
	int read_fd;
};

/* File descriptors are a limited resource. Since the number of file descriptors
 * open at a time grows with the number of tests being run, the test count must
 * be limited based on the open file descriptor limit. This function gets the
 * approximate test limit. */
size_t get_max_tests(void);

/* Run n_tests tests simultaneously. all the `name` and `fun` members must be
 * initialized. On success, 0 is returned. -1 means failure. Test output is
 * printed to stdout. The maximum test run time is `timeout` seconds. n_tests
 * should be less than or equal to get_max_tests(). */
int run_tests(struct test *tests, size_t n_tests, int timeout);

#endif /* TEST_H_ */
