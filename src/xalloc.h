#ifndef XALLOC_H_
#define XALLOC_H_

#include <stddef.h>

void *xmalloc(size_t size);

void *xrealloc(void *mem, size_t size);

#endif /* XALLOC_H_ */
