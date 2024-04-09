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
 * ./ledAPP <filename> <0/OFF:1/ON>
 * ./ledAPP /dev/dtsled 1 : LEDON
 * ./ledAPP /dev/dtsled 0 : LEDOFF
 */

#define LEDOFF  0
#define LEDON   1

/* MAIN Function */
int main(int argc, char *argv[])
{
    int cnt = 0;            /* In order to Check APP access CPU resource */
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

    /* Simulate APP access CPU Resource 5 seconds */
    while(1)
    {
        sleep(5);   /* Delay 5 seconds */
        cnt++;
        printf("APP Running times: %d\r\n", cnt);
        if(cnt >= 5) break;
    }
    printf("APP Runninf Finished~\r\n");


    /* Everything Normal */
    close(fd);
    return 0;
}
