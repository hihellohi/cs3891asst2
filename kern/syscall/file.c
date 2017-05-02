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

// TODO: Take in 3rd argument register and give to vfs_open (mode_t)
// Although this is not even implemented in OS161 so whatever...
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
	file->references = 1;
	curproc->descriptor_table[descriptor] = file;

	return 0;
}

void open_std(void) {
	char con1[] = "con:", con2[] = "con:";
	KASSERT(open(con1, O_WRONLY, 1) == 0); 
	KASSERT(open(con2, O_WRONLY, 2) == 0);
}

int sys_open(userptr_t filename, int flags, int *ret) {
	if (filename == NULL) {
		return EFAULT;
	}
	char *kfilename = kmalloc((PATH_MAX + 1)*sizeof(char)); //check that this is not null
	size_t got;
	int result = copyinstr(filename, kfilename, PATH_MAX + 1, &got);
	if (result) { //free kfilename
		return result;
	}

	int i;
	for (i = 0; i < OPEN_MAX; i++) {
		if (curproc->descriptor_table[i] == NULL) {
			break;
		}
	}
	if (i == OPEN_MAX) { //free kfilename
		return EMFILE;
	}

	int err;
	if((err = open(kfilename, flags, i))){ //free kfilename ... though admittedly this one might be my fault
		return err;
	}

	*ret = i;
	return 0;
}

int sys_close(int filehandler) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}

	struct open_file *file = curproc->descriptor_table[filehandler];

	lock_acquire(file->lock_ptr);
	curproc->descriptor_table[filehandler] = NULL;
	file->references -= 1;

	if(!(file->references)) {
		lock_release(file->lock_ptr); //This is the last reference to this open file
		vfs_close(file->v_ptr);
		lock_destroy(file->lock_ptr);
		kfree(file);
	}
	else{
		lock_release(file->lock_ptr);
	}

	return 0;
}

int sys_read(int filehandler, userptr_t buf, size_t size, int *ret) {
	if(filehandler < 0 || filehandler >= OPEN_MAX || !curproc->descriptor_table[filehandler]) {
		return EBADF;
	}
	struct open_file *file = curproc->descriptor_table[filehandler];

	int how = file->open_flags & O_ACCMODE;
	if (how == O_WRONLY) {
		return EBADF;
	}

	struct iovec iov;
	struct uio myuio;

	lock_acquire(file->lock_ptr);
	off_t old_offset = file->offset;

	uio_uinit(&iov, &myuio, buf, size, file->offset, UIO_READ);

	int result = VOP_READ(file->v_ptr, &myuio);
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
	int how = file->open_flags & O_ACCMODE;
	if (how == O_RDONLY) {
		return EBADF;
	}

	struct iovec iov;
	struct uio myuio;

	lock_acquire(file->lock_ptr);
	off_t old_offset = file->offset;

	uio_uinit(&iov, &myuio, buf, size, file->offset, UIO_WRITE);

	int result = VOP_WRITE(file->v_ptr, &myuio);
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

	if(oldfd == newfd){
		return 0;
	}

	if(curproc->descriptor_table[newfd]){
		int result = sys_close(newfd);
		if(result){
			return result;
		}
	}

	struct open_file *file = curproc->descriptor_table[newfd] = curproc->descriptor_table[oldfd];
	lock_acquire(file->lock_ptr);
	file->references++;
	lock_release(file->lock_ptr);

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
	if((result = copyin(whence_ptr, &whence, sizeof(int)))) {
		return result;
	}

	switch(whence){
		case SEEK_SET:
			if(pos < 0){
				return EINVAL;
			}
			lock_acquire(file->lock_ptr);
			*ret = file->offset = pos;
			lock_release(file->lock_ptr);
			break;

		case SEEK_CUR:
			lock_acquire(file->lock_ptr);
			if(file->offset + pos < 0){
				lock_release(file->lock_ptr);
				return EINVAL;
			}
			*ret = file->offset += pos;
			lock_release(file->lock_ptr);
			break;
		
		case SEEK_END:
			if(stats.st_size + pos < 0){
				return EINVAL;
			}
			lock_acquire(file->lock_ptr);
			*ret = file->offset = stats.st_size + pos; 
			lock_release(file->lock_ptr);
			break;

		default:
			return EINVAL;
	}
	
	return 0;
}

