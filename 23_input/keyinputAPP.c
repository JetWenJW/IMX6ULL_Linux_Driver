#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./imx6uirqAPP <filename> <0/OFF:1/ON>
 * ./imx6uirqAPP /dev/imx6uirq
 */


/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    unsigned char data;

    if(argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);

    /* Some Error Happen */
    if(fd < 0)
    {
        printf("File %s open fail\r\n", filename);
        return -1;
    }

    /* Read KEY Value */
    while(1)
    {
        ret = read(fd, &data, sizeof(data));
        if(ret < 0)
        {

        }
        else
        {
            if(data)
            {
                printf("key value = %#X\r\n",data);
            }
        }
    }

    /* Everything Normal */
    close(fd);
    return 0;
}
