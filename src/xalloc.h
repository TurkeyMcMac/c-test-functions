#ifndef XALLOC_H_
#define XALLOC_H_

#include <stddef.h>

/* Like malloc, but abort on failure. */
void *xmalloc(size_t size);

/* Like realloc, but abort on failure. */
void *xrealloc(void *mem, size_t size);

#endif /* XALLOC_H_ */
