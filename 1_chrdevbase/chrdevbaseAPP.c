#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * argc : the number of Arguments
 * argv[] : the arguments of actions,typically char data type
 * ./chardevbaseAPP <filesname> <1 : Read> <2 : Write>
 * ./chardevbaseAPP/dev/chardevbase 1 Read
 * ./chardevbaseAPP/dev/chardevbase 2 Write
 */
int main(int argc, char *argv[])
{
    int ret = 0;
    int fd = 0;
    char *filename;
    char readbuffer[100], writebuffer[100];
    static char userdata[] = {"User Data!"};


    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);

    if(fd < 0)
    {
        printf("Can't Open files %s\r\n", filename);
        return -1;
    }
    
    if(atoi(argv[2]) == 1)/* Read */
    {
        /* Read Files */
        ret = read(fd, readbuffer, 50);
        if(ret < 0)
        {
            printf("Read Fail %s\r\n", filename);
        }
        else
        {
            printf("APP Read Data :%s\r\n", readbuffer);
        }
    }

    
    if(atoi(argv[2]) == 2)/* Write */
    {
        memcpy(writebuffer, userdata, sizeof(userdata));
        /* Write Files */
        ret = write(fd, writebuffer, 50);
        if(ret < 0)
        {
            printf("Write Fail %s\r\n", filename);
        }
        else
        {
            printf("Write successed~");
        }

    }

    /* Write Files */
    ret = write(fd, writebuffer, 50);
    if(ret < 0)
    {
        printf("Write Fail %s\r\n", filename);
    }

    /* Close Files */
    ret = close(fd);
    if(ret < 0)
    {
        printf("Close Fail %s\r\n", filename);
    }


    return 0;
}
