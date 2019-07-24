#include "options.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct options options;

static void make_regex(const char *prog_name, regex_t *re, const char *pat)
{
	int err = regcomp(re, pat, REG_NOSUB);
	if (err) {
		char errbuf[256];
		regerror(err, re, errbuf, sizeof(errbuf));
		fprintf(stderr, "%s: Regular expression error: %s\n",
			prog_name, errbuf);
		exit(EXIT_FAILURE);
	}
}

static void print_usage(const char *prog_name, FILE *to)
{
	fprintf(to, "Usage: %s [-h] [-l] [-n pat]... [-p num] [-v] [--] file\n",
		prog_name);
}

static void print_help(const char *prog_name, FILE *to)
{
	print_usage(prog_name, to);
	fprintf(to,
"  -h      Print this help information and exit.\n"
"  -l      Just list test names; run no tests.\n"
"  -n pat  Add the Basic Regular Expression <pat> to the matching list. Only\n"
"          tests which match one of these patterns are run.\n"
"  -p num  Set the number of test-running processes to <num>. If unset, it\n"
"          equals the number of tests.\n"
"  -v      Print version information and exit.\n"
	);
}

static void print_version(const char *prog_name, FILE *to)
{
	fprintf(to, "%s version 0.0.0\n", prog_name);
}

void parse_options(int argc, char *argv[]) {
	static const char opts[] =
		"h"  // print help
		"l"  // just list test names
		"n:" // add name pattern
		"p:" // set number of test processes
		"v"  // print version
	;
	size_t name_pats_cap = 0;
	options.n_name_pats = 0;
	options.name_pats = NULL;
	options.n_procs = -1;
	options.just_list = false;
	char opt;
	while ((opt = getopt(argc, argv, opts)) >= 0) {
		switch (opt) {
		case 'h':
			print_help(argv[0], stdout);
			exit(EXIT_FAILURE);
		case 'l':
			options.just_list = true;
			break;
		case 'n':
			make_regex(argv[0], (regex_t *)GROW(options.name_pats,
				options.n_name_pats, name_pats_cap, 1), optarg);
			break;
		case 'p':
			options.n_procs = atoi(optarg);
			break;
		case 'v':
			print_version(argv[0], stdout);
			exit(EXIT_FAILURE);
		default:
			print_usage(argv[0], stderr);
			exit(EXIT_FAILURE);
		}
	}
	options.path = argv[optind];
	if (!options.path) {
		fprintf(stderr, "%s: No file provided\n", argv[0]);
		print_usage(argv[0], stderr);
	}
}
