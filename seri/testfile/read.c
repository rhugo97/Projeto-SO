#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main()
{

	int file;
	char c[7]="";
	int i=0;
	file = open("/dev/seri", O_RDWR);
	i=read(file,c,7);
	c[i]='\n';
	printf("Read: %d\n",i);
	printf("String: %s\n",c);
	close(file);

	return 0;
}
