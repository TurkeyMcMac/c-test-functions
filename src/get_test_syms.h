#ifndef GET_TEST_SYMS_H_
#define GET_TEST_SYMS_H_

#include <stddef.h>

/* Get possible test function symbols from `path`. Candidates are appended to
 * `*names` which already contains `*n_names` names and is allocated to a size
 * if `*names_cap` (times the item size.) */
int get_test_syms(const char *path, char ***names, size_t *n_names,
	size_t *names_cap);

#endif /* GET_TEST_SYMS_H_ */
