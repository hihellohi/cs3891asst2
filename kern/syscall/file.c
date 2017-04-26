#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <proc.h>
#include <syscall.h>
#include <copyinout.h>

/*
 * Add your file-related functions here ...
 */

int sys_open(userptr_t filename, int flags, userptr_t *ret) {
	(void)filename;
	(void)flags;
	(void)ret;

	curproc->descriptor_table[0] = NULL;
	
	return 0;
}

int sys_close(int file) {
	(void)file;
	panic("delete me");

	return 0;
}

void file_bootstrap(void) {
	return;
}

