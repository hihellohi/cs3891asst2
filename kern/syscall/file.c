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

int sys_open(userptr_t filename, int flags, int *ret) {
	char *path = (char *)filename;
	struct vnode *v;
	int result = vfs_open(path, flags, 0, &v);
	if (result) {
		return result;
	}

    int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (curproc->descriptor_table[i] == NULL) {
			break;
		}
	}
	if (i == OPEN_MAX) {
		return EMFILE;
	}

	struct open_file *file = kmalloc(sizeof(struct open_file*));
 
	file->v_ptr = v;
	file->lock_ptr = lock_create("open file lock");
	file->offset = 0;
	file->open_flags = flags;

	curproc->descriptor_table[i] = file;
    *ret = i;
	return 0;
}

int sys_close(int filehandler) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}

	vfs_close(curproc->descriptor_table[filehandler]->v_ptr);
	lock_destroy(curproc->descriptor_table[filehandler]->lock_ptr);

	curproc->descriptor_table[filehandler] = NULL;

	return 0;
}

// TODO: Check if open_flags is valid, end of file?
int sys_read(int filehandler, userptr_t buf, size_t size) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}
    struct open_file *file = curproc->descriptor_table[filehandler];

	void *kernal_buf = kmalloc(size);
    struct iovec iov;
    struct uio myuio;
    uio_kinit(&iov, &myuio, kernal_buf, sizeof(kernal_buf), file->offset, UIO_READ);
    int result = VOP_READ(file->v_ptr, &myuio);
	if (result) {
		return result;
	}
    result = copyout(kernal_buf, buf, size);
	if (result) {
		return result;
	}
    file->offset = myuio.uio_offset;
    return 0;
}

int sys_write(int filehandler, userptr_t buf, size_t size) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}
    struct open_file *file = curproc->descriptor_table[filehandler];

	void *kernal_buf = kmalloc(size);
    int result = copyin(buf, kernal_buf, size);
	if (result) {
		return result;
	}

    struct iovec iov;
    struct uio myuio;
    uio_kinit(&iov, &myuio, kernal_buf, sizeof(kernal_buf), file->offset, UIO_WRITE);
    result = VOP_WRITE(file->v_ptr, &myuio);
	if (result) {
		return result;
	}
    file->offset = myuio.uio_offset;
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

