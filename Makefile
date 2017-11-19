obj-m += ofs.o

all: ioctl
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

ioctl: ioctl.c ofs.h
	gcc -Wall -g -o ioctl ioctl.c

device: all makedev.sh
	sudo ./makedev.sh

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f ioctl openFileSearchDev