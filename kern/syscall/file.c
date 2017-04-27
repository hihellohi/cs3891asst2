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

static int open(char *filename, int flags, int descriptor){
	struct open_file *file = kmalloc(sizeof(struct open_file*));
	if(!file){
		return ENFILE;
	}
 
	int result = vfs_open(filename, flags, 0, &file->v_ptr);
	if (result) {
		kfree(file);
		return result;
	}

	if(!(file->lock_ptr = lock_create("open file lock"))) {
		vfs_close(file->v_ptr);
		kfree(file);
		return ENFILE;
	}

	file->offset = 0;
	curproc->descriptor_table[descriptor] = file;

	return 0;
}

void sys_open_std(void) {
	char con1[] = "con:", con2[] = "con:";
	open(con1, O_WRONLY, 1); 
	open(con2, O_WRONLY, 2); 
}

int sys_open(userptr_t filename, int flags, int *ret) {
	char *path = (char *)filename;

    int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (curproc->descriptor_table[i] == NULL) {
			break;
		}
	}
	if (i == OPEN_MAX) {
		return EMFILE;
	}

	int err;
	if((err = open(path, flags, i))){
		return err;
	}

    *ret = i;
	return 0;
}

int sys_close(int file) {
	if(file < 0 || file >= OPEN_MAX || !curproc->descriptor_table[file]) {
		return EBADF;
	}

	vfs_close(curproc->descriptor_table[file]->v_ptr);
	lock_destroy(curproc->descriptor_table[file]->lock_ptr);
	kfree(curproc->descriptor_table[file]);

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

int sys_lseek(int fd, off_t pos, int whence, off_t *ret){
	(void)fd;
	(void)pos;
	(void)whence;
	(void)ret;
	
	return 0;
}

