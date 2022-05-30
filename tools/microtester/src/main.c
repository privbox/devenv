#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <sys/signal.h>
#include <errno.h>
#include <sys/mman.h>

#include "kerncall.h"

static unsigned long _fastcall0(unsigned long n) {
	unsigned long ret;
	__asm__ __volatile__ (
		"call *%2"
		: "=a"(ret)
		: "a"(n), "m"(kerncall_gate)
		: "rcx", "memory"
	);
	return ret;
}


static void _fastcall0v(unsigned long n) {
	__asm__ __volatile__ (
		"call *%1"
		:
		: "a"(n), "m"(kerncall_gate)
		: "rcx", "r11", "memory"
	);
}

static void _syscall0v(unsigned n) {
	__asm__ __volatile__ (
		"syscall"
		:
		: "a"(n), "m"(kerncall_gate)
		: "rcx", "r11", "memory"
	);
}

int _getpid(void) {
	return _fastcall0(39);
}

uint64_t rdtsc()
{
   uint32_t hi, lo;
   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
   return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

uint64_t user_res = 0, priv_res = 0, call_res = 0;

#define N 10000000UL

static int work_priv(uint64_t *ctr) {
	*ctr = 0;
	for (int i = 0; i < N; i++) {
		uint64_t start = rdtsc();
		_fastcall0v(1024);
		*ctr += rdtsc() - start;
	}
}

static int work_user(uint64_t *ctr) {
	*ctr = 0;
	for (int i = 0; i < N; i++) {
		uint64_t start = rdtsc();
		_syscall0v(1024);
		*ctr += rdtsc() - start;
	}
}

static void dummy_call(void) {
	__asm__ __volatile__ ("":::"memory");
}

static int work_call(uint64_t *ctr) {
	*ctr = 0;
	for (int i = 0; i < N; i++) {
		uint64_t start = rdtsc();
		dummy_call();
		*ctr += rdtsc() - start;
	}
}

static int test_piot_spawn(void)
{
	int err = kerncall_spawn((uintptr_t) work_priv, (unsigned long) &priv_res);
	work_user(&user_res);
	work_call(&call_res);
	printf("priv = %ld, per call %ld\n", priv_res, priv_res / N);
	printf("user = %ld, per call %ld\n", user_res, user_res/ N);
	printf("call = %ld, per call %ld\n", call_res, call_res/ N);
	return err;
}

int main(int argc, char **argv)
{
	kerncall_setup();
	test_piot_spawn();
	return 0;
}
