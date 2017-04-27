/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

struct vnode;
struct lock;

/*
 * Put your function declarations and data types here ...
 */

struct open_file {
	struct vnode *v_ptr;
	struct lock *lock_ptr;
    off_t offset;
    int open_flags;
	int references;
};

void open_std(void);

#endif /* _FILE_H_ */
