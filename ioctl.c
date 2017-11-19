
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include <sys/types.h>

#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <sys/ioctl.h>		/* ioctl */

#include "ofs.h"

#define DEFAULT_REQUEST_SIZE 5


int main(int argc, char* argv[])
{
	int file_desc, result_count;
	int request_size = DEFAULT_REQUEST_SIZE;

	if(argc < 3 || argc > 4){
		printf("./ioctl <operation> <argument> <request_size> (optional)\n");
		printf("./ioctl more <request_size>\n");
		exit(-1);
	}

	if(argc == 4){
		request_size = atoi(argv[3]);
	}

	file_desc = open(DEVICE_FILE_NAME, 0);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	if(strcmp(argv[1],"-p")==0){

		printf("OFS_PID\n");
		unsigned pid = atoi(argv[2]);
		printf("%d\n",pid);
		ioctl(file_desc, OFS_PID, pid);

	} else if (strcmp(argv[1],"-u")==0){

		printf("OFS_UID\n");
		unsigned uid = atoi(argv[2]);
		printf("%d\n",uid);
		ioctl(file_desc, OFS_UID, uid);

	} else if (strcmp(argv[1],"-o")==0){

		printf("OFS_OWNER\n");
		unsigned uid = atoi(argv[2]);
		printf("%d\n",uid);
		ioctl(file_desc, OFS_OWNER, uid);

	} else if (strcmp(argv[1],"-n")==0){

		printf("OFS_NAME\n");
		printf("%s\n",argv[2]);
		char* name = argv[2];
		ioctl(file_desc, OFS_NAME,name);

	} else if (strcmp(argv[1],"more")==0) {

		printf("more\n");
		printf("%s\n",argv[2]);
		request_size = atoi(argv[2]);

	} else {

		printf("invalid operation\n");
		close(file_desc);
		exit(-1);

	}

	struct ofs_result* ofs_results;

	char buffer[sizeof(struct ofs_result)*request_size];

	result_count = read(file_desc, buffer, request_size);

	ofs_results = (struct ofs_result*) buffer;

	char out[256];

	for(int i = 0; i<result_count;i++){

		sprintf(out, "pid(%d),uid(%u),owner(%u),permissions(%d),name(%s),fsize(%u),inode_no(%lu)\n",
			ofs_results[i].pid,
			ofs_results[i].uid,
			ofs_results[i].owner,
			ofs_results[i].permissions,
			ofs_results[i].name,
			ofs_results[i].fsize,
			ofs_results[i].inode_no);

		printf("%s\n",out);
	}

	close(file_desc);
}