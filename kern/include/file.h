/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>


/*
 * Put your function declarations and data types here ...
 */

struct vnode;


struct open_file {
	struct vnode *v_ptr;
	struct uio *f_ptr;
}

struct open_file **open_files;

#endif /* _FILE_H_ */
