
#ifndef OPENFILESEARCHDEV_H
#define OPENFILESEARCHDEV_H

#include <linux/ioctl.h>

#define MAJOR_STATIC 100
#define MAJOR_DYNAMIC 0

#define DRIVER_AUTHOR "Michael Ziegler <ziegler0@hm.edu>"
#define DRIVER_DESC   "Open file search"

#define OFS_PID 0
#define OFS_UID 1
#define OFS_OWNER 3
#define OFS_NAME 4

#define DEVICE_FILE_NAME "openFileSearchDev"

struct ofs_result {
	pid_t pid;
	uid_t uid;
	uid_t owner;
	unsigned short permissions;
	char name[256];
	unsigned int fsize;
	unsigned long inode_no;
};

#endif