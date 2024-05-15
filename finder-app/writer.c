#include "stdio.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

int main (int argc, char* argv[])
{
	openlog(NULL,0,LOG_USER);
        if(argc != 3)
        {
		syslog(LOG_ERR,"Invalid Number of arguments: %d", argc);
                printf("There should be two arguments. 1) Filename 2) Text to write.\n");
                return 1;
        }

        int fd;
        fd = creat(argv[1],0644);
        if(fd==-1)
        {
		syslog(LOG_ERR,"Error creating file: %s", argv[1]);
                printf("Error creating file\n");
                return 1;
        }

        ssize_t nr;
        size_t count;
        count = strlen(argv[2]);
        nr = write(fd, argv[2], count);
        if(nr == -1)
	{
		syslog(LOG_ERR,"Error writing to file: %s",argv[1]);
                printf("Error writing.\n");
	}
        else if(nr != count)
	{
		syslog(LOG_ERR,"Possible error writing to file: %s",argv[1]);
                printf("Possible error writing.\n");
	}

        if(close(fd) == -1)
	{
		syslog(LOG_ERR,"Error closing file: %s",argv[1]);
                printf("Error closing file.\n");
	}

	syslog(LOG_DEBUG, "Writing %s to %s\n",argv[2],argv[1]);
	closelog();

        return 0;
}

