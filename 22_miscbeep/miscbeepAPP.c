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
 * ./miscbeepAPP <filename> <0/OFF:1/ON>
 * ./miscbeepAPP /dev/miscbeep 1 : BEEP_ON
 * ./miscbeepAPP /dev/miscbeep 0 : BEEP_OFF
 */



/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned char databuffer[1];

    if(argc < 3)
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

    databuffer[0] = atoi(argv[2]);      /* Change data type from Char to int */
    retvalue = write(fd, databuffer, sizeof(databuffer));

    /* Some Error Happen */
    if(retvalue < 0)
    {
        printf("LED control Fail\r\n");
        close(fd);
        return -1;
    }

    /* Everything Normal */
    close(fd);
    return 0;
}