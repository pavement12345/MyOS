#ifndef MYOS_HEAP_H
#define MYOS_HEAP_H

#include <stdbool.h>
#include <stddef.h>

void *kernel_heap_allocate(size_t size);
bool kernel_heap_release(void *pointer);

#endif
