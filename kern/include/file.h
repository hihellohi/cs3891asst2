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

/*
 * Put your function declarations and data types here ...
 */

struct descriptor {
	int descriptor_number;
	struct open_file *file;
};


#endif /* _FILE_H_ */
