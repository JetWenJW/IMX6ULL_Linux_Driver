#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>


/*
 * argc : Argument of APP
 * argv[] : char data type, body of Argument
 * ./icm20608APP <filename> <0/OFF:1/ON>
 * ./icm20608APP /dev/icm20608
 * Purpose: Use SPI Trigger ICM20608
 */

int main(int argc, char *argv[])
{
	int fd;
	char *filename;
	signed int databuf[7];
	unsigned char data[14];
	signed int gyro_x_adc, gyro_y_adc, gyro_z_adc;
	signed int accel_x_adc, accel_y_adc, accel_z_adc;
	signed int temp_adc;

	float gyro_x_act, gyro_y_act, gyro_z_act;
	float accel_x_act, accel_y_act, accel_z_act;
	float temp_act;

	int ret = 0;

	if (argc != 2) 
    {
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR);
	if(fd < 0) 
    {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	while (1) 
    {
		ret = read(fd, databuf, sizeof(databuf));
		if(ret == 0) 
        { 			
            /* Read Data Succeed */
			gyro_x_adc  = databuf[0];
			gyro_y_adc  = databuf[1];
			gyro_z_adc  = databuf[2];
			accel_x_adc = databuf[3];
			accel_y_adc = databuf[4];
			accel_z_adc = databuf[5];
			temp_adc    = databuf[6];

			/* Calculate Value */
			gyro_x_act  = (float)(gyro_x_adc)  / 16.4;
			gyro_y_act  = (float)(gyro_y_adc)  / 16.4;
			gyro_z_act  = (float)(gyro_z_adc)  / 16.4;
			accel_x_act = (float)(accel_x_adc) / 2048;
			accel_y_act = (float)(accel_y_adc) / 2048;
			accel_z_act = (float)(accel_z_adc) / 2048;
			temp_act    = ((float)(temp_adc) - 25 ) / 326.8 + 25;


			printf("\r\nOrigin al Value:\r\n");
			printf("gx = %d, gy = %d, gz = %d\r\n", gyro_x_adc, gyro_y_adc, gyro_z_adc);
			printf("ax = %d, ay = %d, az = %d\r\n", accel_x_adc, accel_y_adc, accel_z_adc);
			printf("temp = %d\r\n", temp_adc);
			printf("Precise Value:");
			printf("act gx = %.2f°/S, act gy = %.2f°/S, act gz = %.2f°/S\r\n", gyro_x_act, gyro_y_act, gyro_z_act);
			printf("act ax = %.2fg, act ay = %.2fg, act az = %.2fg\r\n", accel_x_act, accel_y_act, accel_z_act);
			printf("act temp = %.2f°C\r\n", temp_act);
		}
		usleep(100000); /*100ms */
	}
	close(fd);	/* close file */	
	return 0;
}
