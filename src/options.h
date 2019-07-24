#include <regex.h>
#include <stdbool.h>
#include <stddef.h>

extern struct options {
	const char *path;
	regex_t *name_pats;
	size_t n_name_pats;
	int n_procs;
	bool just_list;
} options;

void parse_options(int argc, char *argv[]);
