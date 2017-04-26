#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <limits.h>
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
	char *path = (char *)filename;
	struct vnode *v;
	int result = vfs_open(path, flags, 0, &v);
	if (result) {
		return result;
	}

	for (int i = 0; i < OPEN_MAX; i++ ) {
		if (curproc->descriptor_table[i] == NULL) {
			break;
		}
	}
	if (i == OPEN_MAX) {
		return EMFILE;
	}

	struct open_file *file = kmalloc(sizeof(struct open_file*));
	char buf[128];
	struct iovec iov;
	struct uio myuio;
 
	uio_kinit(&iov, &myuio, buf, sizeof(buf), 0, UIO_READ);
	result = VOP_READ(vn, &myuio);

	file->v_ptr = v;
	file->f_ptr = v->vn_ops->vop_read(v, file_pointer

	curproc->descriptor_table[i] = file;
	*ret = i;
	
	return 0;
}

int sys_close(int file) {
	(void)file;
	panic("delete me");
	proc_create(
	return 0;
}

void file_bootstrap(void) {
	return;
}

