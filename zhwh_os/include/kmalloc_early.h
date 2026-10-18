#include "types.h"

/* Bootstrap allocator only.  The public kernel heap API lives in kmalloc.h. */
void *kmalloc_early(unsigned int size);
