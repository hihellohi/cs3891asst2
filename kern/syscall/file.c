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
	file->open_flags = flags;
	curproc->descriptor_table[descriptor] = file;

	return 0;
}

void open_std(void) {
	char con1[] = "con:", con2[] = "con:";
	open(con1, O_WRONLY, 1); 
	open(con2, O_WRONLY, 2); 
}

int sys_open(userptr_t filename, int flags, int *ret) {
	if (filename == NULL) {
		return EFAULT;
	}
	char *kfilename = kmalloc((PATH_MAX + 1)*sizeof(char));
	size_t got;
	int result = copyinstr(filename, kfilename, PATH_MAX + 1, &got);
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

	int err;
	if((err = open(kfilename, flags, i))){
		return err;
	}

	*ret = i;
	return 0;
}

//TODO should closing duplicate delete all open instances
int sys_close(int filehandler) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}

	vfs_close(curproc->descriptor_table[filehandler]->v_ptr);
	lock_destroy(curproc->descriptor_table[filehandler]->lock_ptr);
	kfree(curproc->descriptor_table[filehandler]);

	curproc->descriptor_table[filehandler] = NULL;

	return 0;
}

// TODO: Check if open_flags is valid, end of file?
// error codes
int sys_read(int filehandler, userptr_t buf, size_t size, int *ret) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}
	struct open_file *file = curproc->descriptor_table[filehandler];

	struct iovec iov;
	struct uio myuio;
	lock_acquire(file->lock_ptr);
	off_t old_offset = file->offset;
	uio_uinit(&iov, &myuio, buf, size, file->offset, UIO_READ);
	int result = file->v_ptr->vn_ops->vop_read(file->v_ptr, &myuio);
	if (result) {
		lock_release(file->lock_ptr);
		return result;
	}
	file->offset = myuio.uio_offset;
	*ret = file->offset - old_offset;
	lock_release(file->lock_ptr);

	return 0;
}

int sys_write(int filehandler, userptr_t buf, size_t size, int *ret) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}
	struct open_file *file = curproc->descriptor_table[filehandler];

	struct iovec iov;
	struct uio myuio;
	lock_acquire(file->lock_ptr);
	off_t old_offset = file->offset;
	uio_uinit(&iov, &myuio, buf, size, file->offset, UIO_WRITE);
	int result = file->v_ptr->vn_ops->vop_write(file->v_ptr, &myuio);
	if (result) {
		lock_release(file->lock_ptr);
		return result;
	}
	file->offset = myuio.uio_offset;
	*ret = file->offset - old_offset;
	lock_release(file->lock_ptr);
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

int sys_lseek(int fd, off_t pos, userptr_t whence_ptr, off_t *ret) {
	struct open_file *file;
	int result;

	if(fd < 0 || fd > OPEN_MAX || !(file = curproc->descriptor_table[fd])){
		return EBADF;
	}

	if(!VOP_ISSEEKABLE(file->v_ptr)){
		return ESPIPE;
	}

	struct stat stats;
	if((result = file->v_ptr->vn_ops->vop_stat(file->v_ptr, &stats))){
		return result;
	}

	int whence;
	if((result = copyin(whence_ptr, &whence, 4))) {
		return result;
	}

	switch(whence){
		case SEEK_SET:
			if(pos < 0){
				return EINVAL;
			}
			*ret = file->offset = pos;
			break;

		case SEEK_CUR:
			if(file->offset + pos < 0){
				return EINVAL;
			}
			*ret = file->offset += pos;
			break;
		
		case SEEK_END:
			if(stats.st_size + pos < 0){
				return EINVAL;
			}
			*ret = file->offset = stats.st_size + pos; 
			break;

		default:
			return EINVAL;
	}
	
	return 0;
}

