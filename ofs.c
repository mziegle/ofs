/*
 *  ofs.c find ofs results filtered by the following criteria
 */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/sched.h>
#include <asm/uaccess.h>	/* for get_user and put_user */
#include <linux/string.h>
#include <linux/fdtable.h>
#include <linux/rcupdate.h>

#include "ofs.h"
#define SUCCESS 0
#define DEVICE_NAME "openFileSearchDev"
#define BUF_LEN 80
#define MAX_RESULT_SIZE 256

// ------ OPTIONS ------

// #define DEBUG

// #define FULL_FILE_PATH

/* 
 * Is the device open right now? Used to prevent
 * concurent access into the same device 
 */
static int Device_Open = 0;
static int major_num;

static int result_count;
static int results_available;
static void* result_ptr;
static struct ofs_result results[MAX_RESULT_SIZE];
static int initial_ioctl;

pid_t param_pid;
uid_t param_uid;
uid_t param_owner;
char param_filename[64];


MODULE_LICENSE("GPL");


MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE(DEVICE_FILE_NAME);

/* 
 * This is called whenever a process attempts to open the device file 
 */
static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "device_open(%p)\n", file);
#endif

	/* 
	 * We don't want to talk to two processes at the same time 
	 */
	if (Device_Open)
		return -EBUSY;

	Device_Open++;

	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif

	/* 
	 * We're now ready for our next caller 
	 */
	Device_Open--;

	module_put(THIS_MODULE);
	return SUCCESS;
}

/* 
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file,	/* see include/linux/fs.h  */
			   char __user * buffer,	/* buffer to be filled with ofs results*/
			   size_t length,	/* number of ofs results */
			   loff_t * offset)
{

	int delivered_results_count, bytes_not_copied, bytes_delivered;
#ifdef DEBUG
	int offset_index, index;
#endif


	if(!initial_ioctl){

#ifdef DEBUG
		printk(KERN_INFO "No initial iocntl \n");
#endif

		return -ESRCH;
	}

	if(results_available < length){
		delivered_results_count = results_available;
		results_available = 0;
	} else {
		delivered_results_count = length;
		results_available -= length;
	}

#ifdef DEBUG

	offset_index = (result_ptr - (void *) results) / sizeof(struct ofs_result);

	printk(KERN_INFO "device_read(%p,%p,%d)\n", file, buffer, length);

	printk(KERN_INFO "offset index %d\n", offset_index);

	printk(KERN_INFO "DATA\n");

	for(index = offset_index; index < offset_index + delivered_results_count; index++){

		printk(KERN_INFO "pid(%d),uid(%u),owner(%u),permissions(%d),name(%s),fsize(%lld),inode_no(%lu)\n",

					results[index].pid,
					results[index].uid,
					results[index].owner,
					results[index].permissions,
					results[index].name,
					results[index].fsize,
					results[index].inode_no

					);
	}

#endif

	bytes_delivered = delivered_results_count * sizeof(struct ofs_result);

#ifdef DEBUG

	printk(KERN_INFO "bytes request %d \n", bytes_delivered);

#endif

	bytes_not_copied = copy_to_user((void*)buffer,result_ptr,bytes_delivered);

	if(bytes_not_copied > 0){
#ifdef DEBUG
		printk(KERN_INFO "bytes not copied %d \n", bytes_not_copied);
#endif
	}

	result_ptr += bytes_delivered;
	
#ifdef DEBUG
	printk(KERN_INFO "read %d ofs results\n",delivered_results_count);
	printk(KERN_INFO "%d ofs results left\n",results_available);
#endif

	return delivered_results_count;
}

int matches_pid_open_files(pid_t pid, uid_t uid_user,uid_t uid_owner,char* filename){
	return pid == param_pid;
}

int matches_user_open_files(pid_t pid, uid_t uid_user,uid_t uid_owner,char* filename){
	return uid_user == param_uid;
}

int matches_owner_open_files(pid_t pid, uid_t uid_user,uid_t uid_owner,char* filename){
	return uid_owner == param_owner;
}

int matches_filename(pid_t pid, uid_t uid_user,uid_t uid_owner,char* filename){
	return strcmp(filename,param_filename)==0;
}

/**
 * Iteraters over all tasks and applies the given filter to it.
 */
void iterate_over_tasks(int (*matcher) (pid_t, uid_t ,uid_t ,char* ))
{   
	
	struct task_struct* task;

	rcu_read_lock();

	result_ptr = (void*) results;
	result_count = 0;

	for_each_process(task){

	    /* Open file information */
	    struct files_struct* open_files;

	    /* file descriptor table */
	    struct fdtable* fd_table;

	    /*Open fds */
	    struct file** fd;

	    /*actual file*/
	    struct file* file;

	    /* current file index */
	    int file_index = 0;

	    /* file owner struct */
	    // struct fown_struct* file_owner;
	    uid_t file_owner;

	    /* the files path*/
	    struct path* f_path;

	    /* file system directory entry */
	    struct dentry* dentry;

	    /* file system file entry*/
	    struct inode* d_inode;

	    /* file permissions */
	    unsigned short opflags;

	    /* file size */
	    long long f_size;

	    /* inode number */
	    unsigned long inode_no;

	    /* file name */
	    char* file_name;

#ifdef FULL_FILE_PATH
		char buffer[256];
#endif

#ifdef DEBUG
		printk(KERN_INFO "task: %s", task->comm);
#endif

	    open_files = task->files;
	    fd_table = open_files->fdt;
	    fd = fd_table->fd;

	    if(fd != NULL){

	    	file = fd[file_index];

		    while(file != NULL && result_count < MAX_RESULT_SIZE){

		    	// NICHT LÖSCHEN!!!!!!!!!

				// PERMISSIONS
				// dentry
				// inode 
				// op flags
		    	f_path = &file->f_path;
				dentry = f_path->dentry;
				d_inode = dentry->d_inode;
				opflags = d_inode->i_opflags;


				// owner
		    	// file_owner = &file->f_owner;
		    	file_owner = d_inode->i_uid.val;

				// FSIZE
				f_size = d_inode->i_size;

				// inode_no
				// dentry
				// inode
				// inode_no
				inode_no = d_inode->i_ino;

				// dentry
				// d_name
				// struct qstr
				// name
				// vorsicht const

				
#ifdef FULL_FILE_PATH
		    	file_name = d_path(&file->f_path,buffer,sizeof(buffer));
#else
		    	file_name = dentry->d_name.name;
#endif
				
				
				if((*matcher)(
						task->pid,
						task->real_cred->euid.val,
						file_owner,
						// file_owner->uid.val,
						file_name
						)
					){

#ifdef DEBUG

					printk(KERN_INFO "pid(%d),uid(%u),owner(%u),permissions(%d),name(%s),fsize(%lld),inode_no(%lu)\n",
					task->pid,
					task->real_cred->euid.val,
					file_owner,
					// file_owner->uid.val,
					opflags,
					file_name,
					f_size,
					inode_no);

#endif
					// Fill struct
					results[result_count].pid = task->pid;
					results[result_count].uid = task->real_cred->euid.val;
					results[result_count].owner = d_inode->i_uid.val; // file_owner->uid.val;
					results[result_count].permissions = opflags;
					strcpy(results[result_count].name,file_name);
					results[result_count].fsize = f_size;
					results[result_count].inode_no = inode_no;

					result_count++;

				}

				file = fd[++file_index];
				
		    }
		}
	}

#ifdef DEBUG
	printk(KERN_INFO "%d results found\n", result_count);
#endif

	results_available = result_count;
	initial_ioctl = 1;

	rcu_read_unlock();

}



/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
long device_ioctl(	/* see include/linux/fs.h */
		 struct file *file,	/* ditto */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)
{

	/* 
	 * Switch according to the ioctl called 
	 */
	switch (ioctl_num) {
	case OFS_PID:

		param_pid = (pid_t) ioctl_param;

		#ifdef DEBUG
			printk(KERN_INFO "OFS_PID : pid(%d)\n", param_pid);
		#endif

		iterate_over_tasks(matches_pid_open_files);

		break;

	case OFS_UID:

		param_uid = (uid_t) ioctl_param;

		#ifdef DEBUG
			printk(KERN_INFO "OFS_UID : uid(%d)\n", param_uid);
		#endif

		iterate_over_tasks(matches_user_open_files);

		break;

	case OFS_OWNER:

		
		param_owner = (uid_t) ioctl_param;

		#ifdef DEBUG
			printk(KERN_INFO "OFS_OWNER : uid(%d)\n", param_owner);
		#endif

		iterate_over_tasks(matches_owner_open_files);

		break;

	case OFS_NAME:

		// copy parameter to kernel space
		copy_from_user((void*)param_filename,(void*)ioctl_param,strlen((char*)ioctl_param));

		#ifdef DEBUG
			printk(KERN_INFO "OFS_NAME : name(%s)\n", param_filename);
		#endif

		iterate_over_tasks(matches_filename);

		break;

	default: 

		return -EINVAL;

	} 

	return SUCCESS;
}

/* Module Declarations */

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. 
 */
struct file_operations Fops = {
	.read = device_read,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,	/* a.k.a. close */
};

/* 
 * Initialize the module - Register the character device 
 */
int init_module()
{
	/* 
	 * Register the character device (atleast try) 
	 */
	major_num = register_chrdev(MAJOR_DYNAMIC, DEVICE_NAME, &Fops);
	/* 
	 * Negative values signify an error 
	 */
	if (major_num < 0) {
		printk(KERN_ALERT "%s failed with %d\n",
		       "Sorry, registering the character device ", major_num);
		return major_num;
	}

	printk(KERN_INFO "%s The major device number is %d.\n",
	       "Registeration is a success", major_num);
	printk(KERN_INFO "If you want to talk to the device driver,\n");
	printk(KERN_INFO "you'll have to create a device file. \n");
	printk(KERN_INFO "We suggest you use:\n");
	printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, major_num);
	printk(KERN_INFO "The device file name is important, because\n");
	printk(KERN_INFO "the ioctl program assumes that's the\n");
	printk(KERN_INFO "file you'll use.\n");

	result_ptr = (void*) results;
	result_count = 0;
	initial_ioctl = 0;

	return 0;
}

/* 
 * Cleanup - unregister the appropriate file from /proc 
 */
void cleanup_module()
{
	/* 
	 * Unregister the device 
	 */
	 unregister_chrdev(major_num, DEVICE_NAME);
}