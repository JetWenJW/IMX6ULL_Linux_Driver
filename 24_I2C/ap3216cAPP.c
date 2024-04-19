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
    int data;

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
    
    err = read(fd, &data, sizeof(&data));

    while(1)
    {
        ;
    }


    /* Everything Normal */
    close(fd);
    return 0;
}
