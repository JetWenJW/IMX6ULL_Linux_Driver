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
#include <poll.h>




/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./imx6uirqAPP <filename> <0/OFF:1/ON>
 * ./imx6uirqAPP /dev/imx6uirq
 */


/* MAIN Function */
int main(int argc, char *argv[])
{
    //fd_set readfds;           /* For select finction */
    //struct timeval timeout;   /* For select finction */
    struct pollfd fds;          /* For poll Function   */

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
#if 0
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
#endif

    /* Read KEY Value */
    while(1)
    {

        //ret = select(fd +1, &readfds, NULL, NULL, &timeout);
        fds.fd = fd;
        fds.events = POLLIN;

        ret = poll(&fds, 1, 500);       /* timeout = 500 ms*/

        if(ret == 0)    /* timeour */
        {

        }
        else if(ret < 0)/* Error */
        {

        }
        else            /* Readable */
        {
            if(fds.revents | POLLIN)
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
        }
#if 0
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
#endif
    }





    /* Everything Normal */
    close(fd);
    return 0;
}
