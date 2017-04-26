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

int sys_open(userptr_t filename, int flags, int *ret) {
	(void)filename;
	(void)flags;
	(void)ret;

	curproc->descriptor_table[0] = NULL;
	
	return 0;
}

int sys_close(int file) {
	if(file < 0 || file >= OPEN_MAX || !curproc->descriptor_table[file]) {
		return EBADF;
	}

	vfs_close(curproc->descriptor_table[file]->v_ptr);

	curproc->descriptor_table[file] = NULL;

	return 0;
}

int sys_dup2(int oldfd, int newfd) {
	if(oldfd < 0 || oldfd >= OPEN_MAX ||
			newfd < 0 || newfd >= OPEN_MAX ||
			!curproc->descriptor_table[oldfd]) {
		return EBADF;
	}

	if(curproc->descriptor_table[newfd]){
		sys_close(newfd);
	}

	curproc->descriptor_table[newfd] = curproc->descriptor_table[oldfd];

	return 0;
}

void file_bootstrap(void) {
	return;
}

