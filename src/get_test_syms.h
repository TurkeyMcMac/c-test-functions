#ifndef GET_TEST_SYMS_H_
#define GET_TEST_SYMS_H_

#include <stddef.h>

/* Get possible test function symbols from `path`. The symbols are stored in
 * `names`, and the count goes in `n_names`. If an error occurs, -1 is returned,
 * otherwise 0. */
int get_test_syms(const char *path, char ***names, size_t *n_names);

#endif /* GET_TEST_SYMS_H_ */
