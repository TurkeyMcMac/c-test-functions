#include "options.h"
#include "style.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct options options;

static void make_regex(const char *prog_name, regex_t *re, const char *pat)
{
	int err = regcomp(re, pat, REG_EXTENDED | REG_NOSUB);
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
	fprintf(to, "Usage: %s [-h] [-l] [-n pat] [-p num] [-t sec] [-v] [--]"
			" file...\n",
		prog_name);
}

static void print_help(const char *prog_name, FILE *to)
{
	print_usage(prog_name, to);
	fprintf(to,
"Run test functions in an executable.\n"
"Options:\n"
"  -h      Print this help information and exit.\n"
"  -l      Just list test names; run no tests.\n"
"  -n pat  Match test names with the regular expression <pat>, running only\n"
"          the ones that match. If none is set, all tests run.\n"
"  -p num  Set the number of test-running processes to <num>. If unset, it\n"
"          equals the number of tests. This is capped at some number that\n"
"          depends on system resources.\n"
"  -s      Force output styles (color, etc.) on.\n"
"  -S      Force output styles off.\n"
"  -t sec  Set the maximum test runtime to <sec> seconds. <sec> is a positive\n"
"          integer. After this time, a test fails.\n"
"  -v      Print version information and exit.\n"
"The <file> argument is the source of test function symbols. There may be\n"
"zero or more files given. Tests from multiple files will not be run in\n"
"parallel.\n"
	);
}

static void print_version(const char *prog_name, FILE *to)
{
	fprintf(to, "%s version 0.8.6\n", prog_name);
}

void parse_options(int argc, char *argv[])
{
	static const char opts[] =
		"h"  // print help
		"l"  // just list test names
		"n:" // add name pattern
		"p:" // set number of test processes
		"sS" // Styles on ("s") or off ("S")
		"t:" // test timeout
		"v"  // print version
	;
	options.has_name_pat = false;
	options.n_procs = -1;
	options.just_list = false;
	enum styles_setting styles_setting = STYLES_AUTO;
	int opt;
	while ((opt = getopt(argc, argv, opts)) >= 0) {
		switch (opt) {
		case 'h':
			print_help(argv[0], stdout);
			exit(EXIT_FAILURE);
		case 'l':
			options.just_list = true;
			break;
		case 'n':
			options.has_name_pat = true;
			make_regex(argv[0], &options.name_pat, optarg);
			break;
		case 'p':
			options.n_procs = atoi(optarg);
			break;
		case 's':
			styles_setting = STYLES_ON;
			break;
		case 'S':
			styles_setting = STYLES_OFF;
			break;
		case 't':
			options.timeout = atoi(optarg);
			break;
		case 'v':
			print_version(argv[0], stdout);
			exit(EXIT_FAILURE);
		default:
			print_usage(argv[0], stderr);
			exit(EXIT_FAILURE);
		}
	}
	init_styles(styles_setting);
	options.paths = argv + optind;
	options.n_paths = argc - optind;
}
