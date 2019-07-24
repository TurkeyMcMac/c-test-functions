#include "test.h"
#include "get_test_syms.h"
#include "util.h"
#include "xalloc.h"
#include <libctf.h>
#include <dlfcn.h>
#include <errno.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int err;
	char **names;
	size_t n_names;
	regex_t name_pat;
	err = regcomp(&name_pat, argv[1], REG_NOSUB);
	if (err) {
		char err_buf[256];
		regerror(err, &name_pat, err_buf, sizeof(err_buf));
		fprintf(stderr, "%s: Regular expression error: %s\n",
			argv[0], err_buf);
		exit(EXIT_FAILURE);
	}
	if (get_test_syms(argv[2], &names, &n_names)) system_error(argv[0]);
	struct test *tests = xmalloc(n_names * sizeof(*tests));
	size_t n_tests = 0;
	void *dl = dlopen(argv[2], RTLD_LAZY);
	if (!dl) {
		fprintf(stderr, "%s: %s: %s",
			argv[0], argv[2], dlerror());
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < n_names; ++i) {
		void *sym = dlsym(dl, names[i]);
		if (sym) {
			char *test_name = names[i] + PREFIX_SIZE;
			*(char *)memmem(test_name, strlen(test_name) + 1,
				SUFFIX, SUFFIX_SIZE + 1) = '\0';
			if (!regexec(&name_pat, test_name, 0, NULL, 0)) {
				tests[n_tests].fun = *(test_fun *)&sym;
				tests[n_tests].name = test_name;
				++n_tests;
			}
		}
	}
	if (run_tests(tests, n_tests)) system_error(argv[0]);
	dlclose(dl);
	regfree(&name_pat);
}
