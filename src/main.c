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
	return !options.has_name_pat
	    || !regexec(&options.name_pat, name, 0, NULL, 0);
}

static void do_tests(const char *prog_name, struct test *tests, size_t n_tests)
{
	if (options.n_procs > 0) {
		size_t n_procs = options.n_procs;
		while (n_tests > n_procs) {
			if (run_tests(tests, n_procs, options.timeout))
				system_error(prog_name);
			tests += n_procs;
			n_tests -= n_procs;
		}
	}
	if (run_tests(tests, n_tests, options.timeout)) system_error(prog_name);
}

int main(int argc, char *argv[])
{
	parse_options(argc, argv);
	char **names = NULL;
	size_t n_names = 0, names_cap = 0;
	struct test *tests = NULL;
	size_t n_tests = 0, tests_cap = 0;
	for (size_t i = 0; i < options.n_paths; ++i) {
		char *path = dll_name_to_path(options.paths[i]);
		int errinfo;
		if (get_test_syms(path, &names, &n_names, &names_cap, &errinfo))
			print_test_syms_error(argv[0], errinfo);
		void *dl = dlopen(path, RTLD_LAZY);
		if (!dl) {
			fprintf(stderr, "%s: %s: %s\n",
				argv[0], path, dlerror());
			exit(EXIT_FAILURE);
		}
		free(path);
		for (size_t j = 0; j < n_names; ++j) {
			void *sym = dlsym(dl, names[j]);
			if (sym) {
				char *test_name = names[j] + PREFIX_SIZE;
				test_name[strlen(test_name) - SUFFIX_SIZE] =
					'\0';
				if (name_matches(test_name)) {
					struct test *test = GROW(tests, n_tests,
						tests_cap, 1);
					test->fun = *(test_fun *)&sym;
					test->name = test_name;
				}
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
}
