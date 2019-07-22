#ifndef TEST_H_
#define TEST_H_

#include <stdio.h>
#include <unistd.h>

typedef int (*test_fun)(void);

FILE *start_test(test_fun fun, pid_t *test_pid);

#endif /* TEST_H_ */
