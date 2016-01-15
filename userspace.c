/*         Process experiment  -  userspace part                  */
/*             meryle swartz - July 2015                          */


////////////////////////////////////////////////////////////////////
/*                  Includes and Definitions                      */
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define SUCCESS 0 
#define FAILURE -1
#define DEBUG 1
#define BUF_LEN 128
#define NUM_ELEMENTS 5


////////////////////////////////////////////////////////////////////
/*                  Function Prototypes                           */ 
////////////////////////////////////////////////////////////////////

int read_from_processdata();
int format_and_print(char *buf);
int print_process_block(char **array);
int open_processdata();
void display_help();



////////////////////////////////////////////////////////////////////
/*                         Functions                              */ 
////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) { 

	int status = FAILURE;
	char *fname;

	if(argc < 2) {
		display_help();
		exit(0);
	}

	//in C, sizeof(char) is guaranteed to be 1
	//so no need to multiply the strlen by anything 
	fname = (char *)malloc(strlen(argv[2]));
	if(!fname) { 
		printf("Malloc error, fatal, exiting\n");
		exit(-1);
	}

	sprintf(fname, "%s", argv[2]);

	// get device info and create /dev/whatever if it doesn't exist already
	status = read_from_processdata(fname);

	free(fname);
	return status;
}


void display_help() { 

	printf("USAGE:\n"
		"./userspace -d /path/to/character_device_file (likely /dev/processdata0 if this is linux)\n");
}

int open_processdata(FILE *fd, char *fname) { 

	int status = FAILURE;

#ifdef DEBUG
	printf("file name is %s\n", fname);
#endif

	if( access( fname, F_OK ) != -1 ) {

		if((fd = fopen(fname, "r")) == NULL) {  
			printf("%s\n", strerror(errno));
		} else {
			status = SUCCESS;
		}
	}  
	return status;
}

int close_processdata(FILE *fd) {

	int status;

	status = fclose(fd);
	if(status != SUCCESS) 
		status = FAILURE;

	return status;
}

int print_process_block(char **array) { 

	printf("Process: %s\n", array[1]);
	printf("Process Identifier: %s\n", array[0]);
	printf("Parent Process Identifier: %s\n", array[2]);
	printf("Start Time: %s\n", array[3]);
	printf("Address: %s\n", array[4]);
	return 0;
}

int read_from_processdata(char *fname) { 

	int status = FAILURE;
	FILE *fd;
	size_t len = 0;
	int looped = 0;
	char line[BUF_LEN];
	int i;
	char *p;
	char *array[NUM_ELEMENTS];

	if( access( fname, F_OK ) != -1 ) {

		if((fd = fopen(fname, "r")) == NULL) {  
			printf("%s\n", strerror(errno));
		} 
	}  

	//split into array and print 
	//and yes, it was very tempting to format in the module but it 
	//seemed sloppy somehow.  
	while(1) { 
		p =  fgets(line, BUF_LEN, fd);

		/* line is:
		 PID|Comm|Ppid|Datetime|Address */	
		p = strdup(line);
		for(i = 0; i < NUM_ELEMENTS; i++) { 
			array[i] = strsep(&p,"|");
		}

		//the first token of the first line will always be '1' for init 
		if((strcmp(array[0],"1")) == 0){
			if(looped) { 
				break;
			} else { 
				looped = 1;
			}
		}

		print_process_block(array);

	}

	status = close_processdata(fd);

	return status;

}

