#ifndef TEST_H_
#define TEST_H_

#include <stddef.h>
#include <unistd.h>

typedef int (*test_fun)(void);

struct test {
	const char *name;
	test_fun fun;
	pid_t pid;
	int read_fd; // Read the output from here
};

int run_tests(struct test *tests, size_t n_tests);

#endif /* TEST_H_ */
