#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./ap3216cAPP <filename> <0/OFF:1/ON>
 * ./ap3216cAPP /dev/ap3216c
 * Purpose: Use input event
 */


/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, err;
    char *filename;
    unsigned short data[3];
    unsigned short ir, als, ps;

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
    

    while(1)
    {
        err = read(fd, data, sizeof(data));
        if(err == 0 )/* Read Succeed */
        {
            ir  = data[0];
            als = data[1];
            ps  = data[2];
            printf("AP3216C ir = %d, als = %d, ps = %d\r\n", ir, als, ps);
        }
        usleep(200000);     /* 200ms */
    }

    /* Everything Normal */
    close(fd);
    return 0;
}
