#include "test.h"
#include "get_test_syms.h"
#include "options.h"
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

static bool name_matches(const char *name)
{
	if (options.n_name_pats == 0) return true;
	for (size_t i = 0; i < options.n_name_pats; ++i) {
		if (!regexec(&options.name_pats[i], name, 0, NULL, 0)) {
			return true;
		}
	}
	return false;
}

static void do_tests(const char *prog_name, struct test *tests, size_t n_tests)
{
	if (options.n_procs > 0) {
		size_t n_procs = options.n_procs;
		while (n_tests > n_procs) {
			if (run_tests(tests, n_procs)) system_error(prog_name);
			tests += n_procs;
			n_tests -= n_procs;
		}
	}
	if (run_tests(tests, n_tests)) system_error(prog_name);
}

int main(int argc, char *argv[])
{
	parse_options(argc, argv);
	char **names;
	size_t n_names;
	if (get_test_syms(options.path, &names, &n_names))
		system_error(argv[0]);
	struct test *tests = xmalloc(n_names * sizeof(*tests));
	size_t n_tests = 0;
	void *dl = dlopen(options.path, RTLD_LAZY);
	if (!dl) {
		fprintf(stderr, "%s: %s: %s",
			argv[0], options.path, dlerror());
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < n_names; ++i) {
		void *sym = dlsym(dl, names[i]);
		if (sym) {
			char *test_name = names[i] + PREFIX_SIZE;
			*(char *)memmem(test_name, strlen(test_name) + 1,
				SUFFIX, SUFFIX_SIZE + 1) = '\0';
			if (name_matches(test_name)) {
				tests[n_tests].fun = *(test_fun *)&sym;
				tests[n_tests].name = test_name;
				++n_tests;
			}
		}
	}
	if (options.just_list) {
		for (size_t i = 0; i < n_tests; ++i) {
			puts(tests[i].name);
		}
	} else {
		do_tests(argv[0], tests, n_tests);
	}
	dlclose(dl);
}
