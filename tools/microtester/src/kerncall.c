#ifdef HAVE_KERNCALL
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include <linux/piot.h>

#include "kerncall.h"

#define PIOT_PATH "/sys/kernel/debug/piot"

static __thread int kerncall_fd = -2;

struct kerncall_wrapper_arg {
	int (*entrypoint)(unsigned long);
	unsigned long arg;
	unsigned long fd;
	unsigned long ret;
};

static void kerncall_entrypoint(unsigned long arg) {
	struct kerncall_wrapper_arg *kcarg = (void *) arg;
	kcarg->ret = kcarg->entrypoint(kcarg->arg);
	ioctl(kcarg->fd, PIOT_IOCRET, 0);
}

int kerncall_setup(void)
{
	struct piot_iocinfo info;
	int fd = open(PIOT_PATH, O_RDWR);
	if (fd < 0)
		return -1;
	int ret = ioctl(fd, PIOT_IOCINFO, &info);
	close(fd);
	if (ret)
		return ret;
	kerncall_gate = (void *) info.kern_gate;
	kerncall_avail = 1;
	return 0;
}

long kerncall_spawn(uintptr_t ptr, unsigned long arg)
{
	if (kerncall_fd == -2) {
		kerncall_fd = open(PIOT_PATH, O_RDWR);
	}
	if (kerncall_fd == -1) {
		return -1;
	}

	struct kerncall_wrapper_arg kcarg = {
		.entrypoint = (void *) ptr,
		.arg = arg,
		.fd = kerncall_fd,
	};
	struct piot_iocspawn spawn = {
		.ip = (uintptr_t) kerncall_entrypoint,
		.arg = (uint64_t) &kcarg,
	};
	int err = ioctl(kerncall_fd, PIOT_IOCSPAWN, &spawn);
	if (err)
		return err;
	return kcarg.ret;
}
#endif
void *kerncall_gate;
int kerncall_avail = 0;