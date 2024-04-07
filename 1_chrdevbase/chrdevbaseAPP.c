#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

/*
 * argc : the number of Arguments
 * argv[] : the arguments of actions,typically char data type
 * ./chardevbaseAPP <filesname>
 */
int main(int argc, char *argv[])
{
    int ret = 0;
    int fd = 0;
    char *filename;
    char readbuffer[100], writebuffer[100];


    filename = argv[1];

    fd = open(filename, O_RDWR);

    if(fd < 0)
    {
        printf("Can't Open files %s\r\n", filename);
        return -1;
    }
    /* Read Files */
    ret = read(fd, readbuffer, 50);
    if(ret < 0)
    {
        printf("Read Fail %s\r\n", filename);
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
