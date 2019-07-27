#include <regex.h>
#include <stdbool.h>
#include <stddef.h>

extern struct options {
	/* The path of the executable. Might not contain a slash, which is
	 * relevant to dlopen. */
	char **paths;
	size_t n_paths;
	/* The pattern which a test name must match to be run. If not
	 * `has_name_pat`, all tests are run. */
	regex_t name_pat;
	bool has_name_pat;
	/* Maximum test run time, in seconds. */
	int timeout;
	/* The maximum number of test-running processes at a time. */
	int n_procs;
	/* If true, the tests should be listed, not run. */
	bool just_list;
} options;

/* Parse options into `options`, exiting on failure. */
void parse_options(int argc, char *argv[]);
