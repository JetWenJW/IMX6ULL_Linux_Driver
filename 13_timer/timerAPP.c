#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./timerAPP <filename> 
 * ./timerAPP /dev/timer
 */


#define CLOSE_CMD       _IO(0xEF, 1)           /* Close command */
#define OPEN_CMD        _IO(0xEF, 2)           /* Open Command  */
#define SETPERIOD_CMD   _IOW(0xEF, 3, int)     /* Set Period    */ 

/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, ret = 0;
    char *filename;
    unsigned char databuffer[1];
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];
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
        printf("Input CMD :");
        ret = scanf("%d", &cmd);
        if(ret != 1)    /* In case of Reading fail */
        {
            gets(str);
        }

        if(cmd == 1)            /* Close Timer */
        {
            ioctl(fd, CLOSE_CMD, &arg);
        }
        else if(cmd == 2)       /* Open Timer */
        {
            ioctl(fd, OPEN_CMD, &arg);
        }
        else if(cmd == 3)       /* Set Period */
        {
            printf("Input Timer Period:");
            scanf("%d", &arg);
            if(ret != 1)
            {
                gets(str);
            }
            ioctl(fd, SETPERIOD_CMD, &arg);
        }
    }

    /* Everything Normal */
    close(fd);
    return 0;
}
