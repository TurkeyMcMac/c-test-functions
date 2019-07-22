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
	void *dl = NULL;
	for (size_t i = 0; i < n_names; ++i) {
		if (!dl && !(dl = dlopen(argv[2], RTLD_LAZY))) {
			fprintf(stderr, "%s: %s: %s",
				argv[0], argv[2], dlerror());
			exit(EXIT_FAILURE);
		}
		void *sym = dlsym(dl, names[i]);
		if (sym) {
			char *test_name = names[i] + PREFIX_SIZE;
			*(char *)memmem(test_name, strlen(test_name) + 1,
				SUFFIX, SUFFIX_SIZE + 1) = '\0';
			if (!regexec(&name_pat, test_name, 0, NULL, 0)) {
				test_fun fun = *(test_fun *)&sym;
				pid_t test_pid;
				FILE *o = start_test(fun, &test_pid);
				if (!o) system_error(argv[0]);
				char *line = NULL;
				size_t line_cap = 0;
				ssize_t len;
				printf("-- %s --\n", test_name);
				while ((len = getline(&line, &line_cap, o)) > 0)
				{
					if (line[len - 1] != '\n') {
						line[len] = '\n';
						++len;
					}
					printf("%s:%.*s",
						test_name, (int)len, line);
				}
				int exit_info;
				waitpid(test_pid, &exit_info, 0);
				int exit_code = WEXITSTATUS(exit_info);
				int core_dump = WCOREDUMP(exit_info);
				printf("%s %s   Exit code: %d%s\n",
					test_name,
					exit_code == 0 && !core_dump ?
						"SUCCEEDED" : "FAILED",
					exit_code,
					core_dump ? "   Core dumped" : "");
				if (!WIFEXITED(exit_info)) dlclose(dl);
				dl = NULL;
			}
		}
	}
	if (dl) dlclose(dl);
	regfree(&name_pat);
}

#include <assert.h>

CTF_TEST(test_1,
	printf("test 1\n");
)

CTF_TEST(test_2,
	printf("test 2\n");
	assert(1 == 2);
)

CTF_TEST(test_3,
	printf("test 3\n");
	return 2;
)
