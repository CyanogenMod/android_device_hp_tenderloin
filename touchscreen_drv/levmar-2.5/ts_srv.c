/*
 * This is a userspace touchscreen driver for cypress ctma395 as used
 * in HP Touchpad configured for WebOS.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * The code was written from scrath, the hard math and understanding the
 * device output by jonpry @ gmail
 * uinput bits and the rest by Oleg Drokin green@linuxhacker.ru
 *
 * Copyright (c) 2011 CyanogenMon Touchpad Project.
 *
 * Multitouch detection by deeper-blue Rafael Brune (mail@rbrune.de))on Sep 6th.
 *
 */

#include "/home/jon/touchdroid/linux-2.6.35/include/linux/input.h"
#include "/home/jon/touchdroid/linux-2.6.35/include/linux/uinput.h"
#include "/home/jon/touchdroid/linux-2.6.35/include/linux/hsuart.h"
#include "/home/jon/touchdroid/linux-2.6.35/include/linux/i2c.h"
#include "/home/jon/touchdroid/linux-2.6.35/include/linux/i2c-dev.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/select.h>

#define USE_LEVMAR

#if 1
// This is for Andrioid
#define UINPUT_LOCATION "/dev/uinput"
#else
// This is for webos and possibly other Linuxes
#define UINPUT_LOCATION "/dev/input/uinput"
#endif

/* Set to 1 to print coordinates to stdout. */
#define DEBUG 0

/* Set to 1 to see raw data from the driver */
#define RAW_DATA_DEBUG 0

#define WANT_MULTITOUCH 1
#define WANT_SINGLETOUCH 0

#define RECV_BUF_SIZE 1540
#define LIFTOFF_TIMEOUT 20000 /* 20ms */

#define MAX_CLIST 75

unsigned char cline[64];
unsigned int cidx=0;
unsigned char matrix[30][40]; 
int uinput_fd;

int send_uevent(int fd, __u16 type, __u16 code, __s32 value)
{
    struct input_event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    gettimeofday(&event.time, NULL);

    if (write(fd, &event, sizeof(event)) != sizeof(event)) {
        fprintf(stderr, "Error on send_event %d", sizeof(event));
        return -1;
    }

    return 0;
}


struct candidate {
	int pw;
	int i;
	int j;
};

struct touchpoint {
	int pw;
	double i;
	double j;
};

int tpcmp(const void *v1, const void *v2)
{
    return ((*(struct candidate *)v2).pw - (*(struct candidate *)v1).pw);
}

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#if WANT_MULTITOUCH

#define SD 0.66
void runlm(int, double*, double*, double*);

void generate_submatrix(int radius, int locx, int locy, int* ofstx, int* ofsty, double* submatrix)
{
	int i,j,k,l,stride;
	double guess[5];

	stride = radius*2+1;
	i=locx; j=locy;

	i=MAX(0,i-radius);
	j=MAX(0,j-radius);
	i=MIN(i,29-radius*2);
	j=MIN(j,39-radius*2);

	for(k=0; k < stride; k++)
	{
		for(l=0; l < stride; l++)
			submatrix[k*stride+l] = matrix[i+k][j+l];
	}

	*ofstx = i;
	*ofsty = j;
}

void process_submatrix_levmar(int locx, int locy, int radius, double* submatrix, double* results)
{
	int i,j, tval, peak=0;
	int total=0, xtotal=0,ytotal=0;
	double guess[5];
	int stride = 1+2*radius;

#if DEBUG
	printf("Sub Matrix: \n");
#endif
	for(i=0; i < stride; i++)
	{
		for(j=0; j < stride; j++)
		{
			tval = submatrix[i*stride+j];
#if DEBUG
			printf("%2.2X ", tval);
#endif
			if(tval > peak) peak = tval;
			total+=tval;
			xtotal += tval*i;
			ytotal += tval*j;
		}
#if DEBUG
		printf("\n");
#endif
	}

	guess[0] = peak;   //Peak by factor
	guess[1] = 0; //Guess that x is zero
	guess[2] = 0; //Guess that y is zero
	guess[3] = SD;

	runlm(radius,guess,submatrix,results);

	results[0] += locx+radius;
	results[1] += locy+radius;
//#if DEBUG
	printf("Coords: %d, %d, %d, %d, %g, %g\n", locx, locy, i, j, results[0], results[1]);
//#endif
}	

void process_submatrix_avg(int locx, int locy, int radius, double* submatrix, double* results)
{
	int i,j,tweight;
	double isum,jsum;
	double avgi,avgj;
	double powered;
	int stride = radius*2+1;

	tweight=0;
	isum=0;
	jsum=0;
	for(i=0; i < stride; i++) {
		for(j=0; j < stride; j++) {
			powered = pow(submatrix[i*stride+j], 1.5);
			tweight += powered;
			isum += powered * i;
			jsum += powered * j;
		}
	}
	avgi = isum / (double)tweight;
	avgj = jsum / (double)tweight;
	results[0] = avgi + locx;
	results[1] = avgj + locy;
}

int is_peak(int i, int j)
{
	int mini,maxi,minj,maxj;
	int tvalue = matrix[i][j];

//	printf("Detect: %d, %d\n", i,j);
	mini = i-1; maxi=i+2;
	minj = j-1; maxj=j+2;
	for(i=MAX(0, mini); i < MIN(30, maxi); i++) {
		for(j=MAX(0, minj); j < MIN(40, maxj); j++) {
//			printf("Matrix: %d, %d: %d - %d\n", i,j,matrix[i][j],tvalue);
			if(matrix[i][j] > tvalue)
			{
//				printf("Not a peak\n");
				return 0;
			}
		}
	}
	return 1;
}

void calc_point()
{
	int i,j, dx, dy, d2;
	double avgi, avgj;
	double levmar_results[2];
	double avg_results[2];
	double submatrix[81];
	int ofstx, ofsty;	

	int tpc=0;
	struct touchpoint tpoint[10];

	int clc=0;
	struct candidate clist[MAX_CLIST];
	
	// generate list of high values
	for(i=0; i < 30; i++) {
		for(j=0; j < 40; j++) {
			if(matrix[i][j] > 10 && clc < MAX_CLIST && is_peak(i,j)) {
//				printf("Peak: %d, %d\n", i, j);
				clist[clc].pw = matrix[i][j];
				clist[clc].i = i;
				clist[clc].j = j;
				clc++;
			}
		}
	}
#if DEBUG
	printf("%d clc\n", clc);
#endif

	// sort candidate list by strength
	qsort(clist, clc, sizeof(clist[0]), tpcmp);

#if DEBUG
	printf("%d %d %d \n", clist[0].pw, clist[1].pw, clist[2].pw);
#endif

	int k, l;
	for(k=0; k < clc; k++) {
		int newtp=1;
		
		int rad=2; // radius around candidate to use for discounting others
		int mini = clist[k].i - rad+1;
		int maxi = clist[k].i + rad;
		int minj = clist[k].j - rad+1;
		int maxj = clist[k].j + rad;
		
		// discard points close to already detected touches
		for(l=0; l<tpc; l++) {
			if(tpoint[l].i > mini && tpoint[l].i < maxi && tpoint[l].j > minj && tpoint[l].j < maxj) newtp=0;
		}
		
		// calculate new touch near the found candidate
		if(newtp && tpc < 10) {
			d2 = 1000;
			for(l=0; l<clc; l++) {
				if(l==k)
					continue;
				dx = tpoint[k].i - tpoint[l].i;
				dy = tpoint[k].j - tpoint[l].j;
				int td2 = dx*dx+dy*dy;
				if(td2 < d2) d2 = td2;
			}

			generate_submatrix(4,clist[k].i,clist[k].j,&ofstx,&ofsty,submatrix);
			process_submatrix_avg(ofstx,ofsty,4,submatrix,avg_results);
			avgi = avg_results[0];
			avgj = avg_results[1];


#ifdef USE_LEVMAR
			if(clist[k].i < 4 || clist[k].j < 4 || clist[k].i > 26 || clist[k].j > 36 || d2 < 16)
			{
				generate_submatrix(2,clist[k].i,clist[k].j,&ofstx,&ofsty,submatrix);

				process_submatrix_levmar(ofstx,ofsty,2,submatrix,levmar_results);

				avgi = levmar_results[0];
				avgj = levmar_results[1];
			}
#endif

			tpc++;

//#if DEBUG
			printf("Coords %d %lf, %lf\n", tpc, avgi, avgj);
//#endif

#if 0
			/* Android does not need this an it simplifies stuff
			 * for us as we don't need to track individual touches
			 */
			send_uevent(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, tpc);
#endif
			send_uevent(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 10);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_X, avgi*768/29);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, 1024-avgj*1024/39);
			send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);


		}
	}

	send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
}
#endif


#if WANT_SINGLETOUCH
void calc_point()
{
	int i,j;
	int tweight=0;
	double xsum=0, ysum=0;
	double avgx, avgy;
	double powered;

	for(i=0; i < 30; i++)
	{
		for(j=0; j < 40; j++)
		{
			if(matrix[i][j] < 3)
				matrix[i][j] = 0;
#if RAW_DATA_DEBUG
			printf("%2.2X ", matrix[i][j]);
#endif

			powered = pow(matrix[i][j],1.5);
			tweight += powered;
			ysum += powered * j;
			xsum += powered * i;
		}
#if RAW_DATA_DEBUG
		printf("\n");
#endif
	}
	avgx = xsum / (double)tweight;
	avgy = ysum / (double)tweight;

#if DEBUG
	printf("Coords %lf, %lf, %d\n", avgx,avgy, tweight);
#endif

	/* Single touch signals */
	send_uevent(uinput_fd, EV_ABS, ABS_X, avgx*768/29);
	send_uevent(uinput_fd, EV_ABS, ABS_Y, 1024-avgy*1024/39);
	send_uevent(uinput_fd, EV_ABS, ABS_PRESSURE, 1);
	send_uevent(uinput_fd, EV_ABS, ABS_TOOL_WIDTH, 10);
	send_uevent(uinput_fd, EV_KEY, BTN_TOUCH, 1);

	send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
	
}
#endif

void put_byte(unsigned char byte)
{
//	printf("Putc %d %d\n", cidx, byte);
	if(cidx==0 && byte != 0xFF)
		return;

	//Sometimes a send is aborted by the touch screen. all we get is an out of place 0xFF
	if(byte == 0xFF && !cline_valid(1))
		cidx = 0;
	cline[cidx++] = byte;
}

int cline_valid(int extras)
{
	if(cline[0] == 0xff && cline[1] == 0x43 && cidx == 44-extras)
	{
//		printf("cidx %d\n", cline[cidx-1]);
		return 1;
	}
	if(cline[0] == 0xff && cline[1] == 0x47 && cidx > 4 && cidx == (cline[2]+4-extras))
	{
//		printf("cidx %d\n", cline[cidx-1]);
		return 1;
	}
	return 0;
}

void consume_line()
{
	int i,j;

	if(cline[1] == 0x47)
	{
		//calculate the data points. all transfers complete
		calc_point();
	}

	if(cline[1] == 0x43)
	{
		//This is a start event. clear the matrix
		if(cline[2] & 0x80)
		{
			for(i=0; i < 30; i++)
				for(j=0; j < 40; j++)
					matrix[i][j] = 0;
		}

		//Write the line into the matrix
		for(i=0; i < 40; i++)
			matrix[cline[2] & 0x1F][i] = cline[i+3];
	}

//	printf("Received %d bytes\n", cidx-1);
		
/*		for(i=0; i < cidx; i++)
			printf("%2.2X ",cline[i]);
		printf("\n");	*/
	cidx = 0;
}

void snarf2(unsigned char* bytes, int size)
{
	int i=0;
	while(i < size)
	{
		while(i < size)
		{
			put_byte(bytes[i]);
			i++;
			if(cline_valid(0))
			{
//				printf("was valid\n");
				break;
			}
		}

		if(i >= size)
			break;

//		printf("Cline went valid\n");
		consume_line();
	}

	if(cline_valid(0))
	{
		consume_line();
//		printf("was valid2\n");
	}
}

void open_uinput()
{
    struct uinput_user_dev device;
    struct input_event myevent;
    int i,ret = 0;

    memset(&device, 0, sizeof device);

    uinput_fd=open(UINPUT_LOCATION,O_WRONLY);
    strcpy(device.name,"HPTouchpad");

    device.id.bustype=BUS_USB;
    device.id.vendor=1;
    device.id.product=1;
    device.id.version=1;

    for (i=0; i < ABS_MAX; i++) {
        device.absmax[i] = -1;
        device.absmin[i] = -1;
        device.absfuzz[i] = -1;
        device.absflat[i] = -1;
    }

    device.absmin[ABS_X]=0;
    device.absmax[ABS_X]=768;
    device.absfuzz[ABS_X]=2;
    device.absflat[ABS_X]=0;
    device.absmin[ABS_Y]=0;
    device.absmax[ABS_Y]=1024;
    device.absfuzz[ABS_Y]=1;
    device.absflat[ABS_Y]=0;
    device.absmin[ABS_PRESSURE]=0;
    device.absmax[ABS_PRESSURE]=1;
    device.absfuzz[ABS_PRESSURE]=0;
    device.absflat[ABS_PRESSURE]=0;
#if WANT_MULTITOUCH
    device.absmin[ABS_MT_POSITION_X]=0;
    device.absmax[ABS_MT_POSITION_X]=768;
    device.absfuzz[ABS_MT_POSITION_X]=2;
    device.absflat[ABS_MT_POSITION_X]=0;
    device.absmin[ABS_MT_POSITION_Y]=0;
    device.absmax[ABS_MT_POSITION_Y]=1024;
    device.absfuzz[ABS_MT_POSITION_Y]=1;
    device.absflat[ABS_MT_POSITION_Y]=0;
    device.absmax[ABS_MT_TOUCH_MAJOR]=1;
    device.absmax[ABS_MT_WIDTH_MAJOR]=100;
#endif


    if (write(uinput_fd,&device,sizeof(device)) != sizeof(device))
    {
        fprintf(stderr, "error setup\n");
    }

    if (ioctl(uinput_fd,UI_SET_EVBIT,EV_KEY) < 0)
        fprintf(stderr, "error evbit key\n");

    if (ioctl(uinput_fd,UI_SET_EVBIT, EV_SYN) < 0)
        fprintf(stderr, "error evbit key\n");

    if (ioctl(uinput_fd,UI_SET_EVBIT,EV_ABS) < 0)
            fprintf(stderr, "error evbit rel\n");

#if 1
    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_X) < 0)
            fprintf(stderr, "error x rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_Y) < 0)
            fprintf(stderr, "error y rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_PRESSURE) < 0)
            fprintf(stderr, "error pressure rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_TOOL_WIDTH) < 0)
            fprintf(stderr, "error tool rel\n");
#endif

//    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_TRACKING_ID) < 0)
//            fprintf(stderr, "error trkid rel\n");

#if WANT_MULTITOUCH
    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_TOUCH_MAJOR) < 0)
            fprintf(stderr, "error tool rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_WIDTH_MAJOR) < 0)
            fprintf(stderr, "error tool rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_POSITION_X) < 0)
            fprintf(stderr, "error tool rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_POSITION_Y) < 0)
            fprintf(stderr, "error tool rel\n");
#endif

#if 1
    if (ioctl(uinput_fd,UI_SET_KEYBIT,BTN_TOUCH) < 0)
            fprintf(stderr, "error evbit rel\n");
#endif

    if (ioctl(uinput_fd,UI_DEV_CREATE) < 0)
    {
        fprintf(stderr, "error create\n");
    }

}

int main(int argc, char** argv)
{
	struct hsuart_mode uart_mode;
	struct i2c_rdwr_ioctl_data i2c_ioctl_data;
	struct i2c_msg i2c_msg;
	int uart_fd, vdd_fd, xres_fd, wake_fd, i2c_fd, nbytes, i; 
	char recv_buf[RECV_BUF_SIZE];
	char i2c_buf[16];
	fd_set fdset;
	struct timeval seltmout;

	uart_fd = open("/dev/ctp_uart", O_RDONLY|O_NONBLOCK);
	if(uart_fd<=0)
	{
		printf("Could not open uart\n");
		return 0;
	}

	open_uinput();

	ioctl(uart_fd,HSUART_IOCTL_GET_UARTMODE,&uart_mode);
	uart_mode.speed = 0x3D0900;
	ioctl(uart_fd, HSUART_IOCTL_SET_UARTMODE,&uart_mode);

	vdd_fd = open("/sys/devices/platform/cy8ctma395/vdd", O_WRONLY);
	xres_fd = open("/sys/devices/platform/cy8ctma395/xres", O_WRONLY);
	wake_fd = open("/sys/user_hw/pins/ctp/wake/level", O_WRONLY);
	i2c_fd = open("/dev/i2c-5", O_RDWR);

	lseek(vdd_fd, 0, SEEK_SET);
	write(vdd_fd, "1", 1);

	lseek(wake_fd, 0, SEEK_SET);
	write(wake_fd, "1", 1);

	lseek(xres_fd, 0, SEEK_SET);
	write(xres_fd, "1", 1);

	lseek(xres_fd, 0, SEEK_SET);
	write(xres_fd, "0", 1);

	usleep(50000);

	ioctl(uart_fd, HSUART_IOCTL_FLUSH, 0x9);

	lseek(wake_fd, 0, SEEK_SET);
	write(wake_fd, "0", 1);

	usleep(50000);

	i2c_ioctl_data.nmsgs = 1;
	i2c_ioctl_data.msgs = &i2c_msg;

	i2c_msg.addr = 0x67;
	i2c_msg.flags = 0;
	i2c_msg.buf = i2c_buf;
	
	i2c_msg.len = 2;
	i2c_buf[0] = 0x08; i2c_buf[1] = 0;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_msg.len = 6;
	i2c_buf[0] = 0x31; i2c_buf[1] = 0x01; i2c_buf[2] = 0x08;
	i2c_buf[3] = 0x0C; i2c_buf[4] = 0x0D; i2c_buf[5] = 0x0A; 
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_msg.len = 2;
	i2c_buf[0] = 0x30; i2c_buf[1] = 0x0F;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_buf[0] = 0x40; i2c_buf[1] = 0x02;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_buf[0] = 0x41; i2c_buf[1] = 0x10;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_buf[0] = 0x0A; i2c_buf[1] = 0x04;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);

	i2c_buf[0] = 0x08; i2c_buf[1] = 0x03;
	ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);	

	lseek(wake_fd, 0, SEEK_SET);
	write(wake_fd, "1", 1);

	while(1)
	{
	//	usleep(50000);
		FD_ZERO(&fdset);
		FD_SET(uart_fd, &fdset);
		seltmout.tv_sec = 0;
		/* 2x tmout */
		seltmout.tv_usec = LIFTOFF_TIMEOUT;

		if (0 == select(uart_fd+1, &fdset, NULL, NULL, &seltmout)) {
			/* Timeout means liftoff, send event */
#if DEBUG
			printf("timeout! sending liftoff\n");
#endif
#if 1
			send_uevent(uinput_fd, EV_ABS, ABS_PRESSURE, 0);
//			send_uevent(uinput_fd, EV_ABS, BTN_2, 0);
			send_uevent(uinput_fd, EV_KEY, BTN_TOUCH, 0);
#endif

#if WANT_MULTITOUCH
//			send_uevent(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, 1);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
			send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
#endif

			send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);

			FD_ZERO(&fdset);
			FD_SET(uart_fd, &fdset);
			/* Now enter indefinite sleep iuntil input appears */
			select(uart_fd+1, &fdset, NULL, NULL, NULL);
			/* In case we were wrongly woken up check the event
			 * count again */
			continue;
		}
			
		nbytes = read(uart_fd, recv_buf, RECV_BUF_SIZE);
		
		if(nbytes <= 0)
			continue;


	/*	printf("Received %d bytes\n", nbytes);
		
		for(i=0; i < nbytes; i++)
			printf("%2.2X ",recv_buf[i]);
		printf("\n");	*/	

		snarf2(recv_buf,nbytes);

	}

	return 0;
}
