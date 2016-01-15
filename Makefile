obj-m+=proctest.o


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	gcc -c userspace.c -o userspace.o
	gcc userspace.o -o userspace 
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	-rm -f userspace.o
	-rm -f userspace 
