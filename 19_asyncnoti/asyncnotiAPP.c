#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./asyncnotiAPP <filename> <0/OFF:1/ON>
 * ./asyncnotiAPP /dev/imx6uirq
 */

int fd;
/* Signal Handler */
static void sigio_signal_func(int num)
{
    int err;
    unsigned int keyvalue = 0;

    read(fd, &keyvalue, sizeof(keyvalue));
    if(err < 0)
    {

    }
    else
    {
        printf("SigIO signal! Key Value = %d\r\n", keyvalue);
    }
}

/* MAIN Function */
int main(int argc, char *argv[])
{
    int ret;
    char *filename;
    unsigned char data;
    int flags = 0;

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

    /*Set Signal Handler Function */
    signal(SIGIO, sigio_signal_func);
    
    fcntl(fd, F_SETOWN, getpid());          /* Set current Thread Get SIGIO */
    flags = fcntl(fd, F_GETFL);             /* Enable Asynchroinize         */
    fcntl(fd, F_SETFL, flags | FASYNC);     /* Asynchronize notice          */

    while(1)
    {
        sleep(2);
    }
    /* Everything Normal */
    close(fd);
    return 0;
}
