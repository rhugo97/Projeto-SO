#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



int main(int argc, char *argv[]){
	int fd=open("/dev/seri",O_RDWR);
	write(fd,argv[1],strlen(argv[1]));
	close(fd);
}
