#include <regex.h>
#include <stdbool.h>
#include <stddef.h>

extern struct options {
	/* The path of the executable. Might not contain a slash, which is
	 * relevant to dlopen. */
	const char *path;
	/* The patterns which a test name must match to be run. If there are 0,
	 * all tests match. Otherwise, one pattern must match. */
	regex_t *name_pats;
	size_t n_name_pats;
	/* Maximum test run time, in seconds. */
	int timeout;
	/* The maximum number of test-running processes at a time. */
	int n_procs;
	/* If true, the tests should be listed, not run. */
	bool just_list;
} options;

/* Parse options into `options`, exiting on failure. */
void parse_options(int argc, char *argv[]);
