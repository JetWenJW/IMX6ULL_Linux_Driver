#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>




/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./imx6uirqAPP <filename> <0/OFF:1/ON>
 * ./imx6uirqAPP /dev/imx6uirq
 */


/* MAIN Function */
int main(int argc, char *argv[])
{
    fd_set readfds;
    struct timeval timeout;
    int fd, ret;
    char *filename;
    unsigned char data;

    if(argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR | O_NONBLOCK);   /* NonBlocking Open File */

    /* Some Error Happen */
    if(fd < 0)
    {
        printf("File %s open fail\r\n", filename);
        return -1;
    }

    /* Read KEY Value */
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 5000000;/* 500ms */
        ret = select(fd +1, &readfds, NULL, NULL, &timeout);
        switch(ret)
        {
            case 0:     /* Timeout */
                printf("select timeout!\r\n");
                break;
            case -1:     /* Error   */
                break;
            default :   /* Readable Data */
                if(FD_ISSET(fd, &readfds))
                {
                    ret = read(fd, &data, sizeof(data));
                    if(ret < 0)
                    {

                    }
                    else
                    {
                        if(data)
                        {
                            printf("key_value = %#X\r\n", data);
                        }
                    }
                }
                break;
        }
    }

    /* Everything Normal */
    close(fd);
    return 0;
}
