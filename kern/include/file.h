/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

struct open_file;
struct vnode;
struct lock;

/*
 * Put your function declarations and data types here ...
 */

struct open_file {
	struct vnode *v_ptr;
	struct lock *lock_ptr;
    off_t offset;
};

struct open_file *open_files[OPEN_TOTAL_MAX];

void file_bootstrap(void);

#endif /* _FILE_H_ */
