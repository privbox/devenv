#ifndef __BENCHMARK_KERNCALL_H
#define __BENCHMARK_KERNCALL_H

#include <stdint.h>
#include <stdio.h>

extern void *kerncall_gate;

#ifdef HAVE_KERNCALL


int kerncall_setup(void);
long kerncall_spawn(uintptr_t ptr, unsigned long arg);

#else // HAVE_KERNCALL



static inline int kerncall_setup(void) { return 0; }
static inline long kerncall_spawn(uintptr_t ptr, unsigned long arg) {
    fprintf(stderr, "using kerncall without -DHAVE_KERNCALL, calling directly\n");
    long (*func)(unsigned long) = (long (*)(unsigned long)) ptr;
    return func(arg);
}
#endif // HAVE_KERNCALL

extern int kerncall_avail;

#endif // __BENCHMARK_KERNCALL_H