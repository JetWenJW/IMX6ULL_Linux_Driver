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
 * ./keyAPP <filename> <0/OFF:1/ON>
 * ./keyAPP /dev/key
 */


#define KEY_0_VALUE     0xF0         
#define INVAKEY         0x00      

/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned char databuffer[1];
    int value;

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
        read(fd, &value, sizeof(value));
        if(value == KEY_0_VALUE)
        {
            printf("KEY0 PRess, value = %d\r\n", value);
        }
    }

    /* Everything Normal */
    close(fd);
    return 0;
}
