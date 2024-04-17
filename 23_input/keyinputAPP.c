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
 * ./keyinputAPP <filename> <0/OFF:1/ON>
 * ./keyinputAPP /dev/input/event1
 * Purpose: Use input event
 */

static struct input_event inputevent;

/* MAIN Function */
int main(int argc, char *argv[])
{
    int fd, err;
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

    while(1)
    {
        err = read(fd, &inputevent, sizeof(&inputevent));
        if(err > 0)/* Read data succeed */
        {
            switch(inputevent.type)
            {
                case EV_KEY :
                    if(inputevent.code < BTN_MISC)  /* KEY (Kernel Define it)*/
                    {
                        printf("KEY %d %s\r\n", inputevent.code, inputevent.value ? "Press" : "Release");
                    }
                    else
                    {
                        printf("Bottom %d %s\r\n", inputevent.code, inputevent.value ? "Press" : "Release");
                    }
                    break;
        
                case EV_SYN :
                    printf("EV_SYN Event\r\n");
                    break;
                case EV_REL :
                    printf("EV_REL Event\r\n");
                    break;
                case EV_ABS :
                    printf("EV_ABS Event\r\n");
                    break;
            }
        }
        else                /* Read Data Fail */
        {
            printf("Fail Read Data\r\n");
        }
    }


    /* Everything Normal */
    close(fd);
    return 0;
}
