#ifndef GET_TEST_SYMS_H_
#define GET_TEST_SYMS_H_

#include <stddef.h>

/* Get possible test function symbols from `path`. Candidates are appended to
 * `*names` which already contains `*n_names` names and is allocated to a size
 * if `*names_cap` (times the item size.) `errinfo` is a pointer to an opaque
 * integer which can only be used by print_test_syms_error. It cannot even be
 * used by print_test_syms_erro unless get_test_syms returned an error. It will
 * always be garbled by get_test_syms, no matter if it succeeded or failed. */
int get_test_syms(const char *path, char ***names, size_t *n_names,
	size_t *names_cap, int *errinfo);

/* Print to stderr a message about why get_test_syms failed, using `prog_name`
 * as the name of the program and `errinfo` for error-specific information. */
void print_test_syms_error(const char *prog_name, int errinfo);

#endif /* GET_TEST_SYMS_H_ */
