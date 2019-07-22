#include <stdlib.h>

void *xmalloc(size_t size)
{
	void *mem = malloc(size ? size : 1);
	if (!mem) abort();
	return mem;
}

void *xrealloc(void *mem, size_t size)
{
	void *new_mem = realloc(mem, size ? size : 1);
	if (!new_mem) abort();
	return new_mem;
}
