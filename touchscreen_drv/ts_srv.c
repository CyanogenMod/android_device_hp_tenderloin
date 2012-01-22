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
 * Multitouch detection by Rafael Brune mail@rbrune.de
 *
 * Copyright (c) 2011 CyanogenMod Touchpad Project.
 *
 *
 */

#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/hsuart.h>
#include <sched.h>
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

#if 1
// This is for Android
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

#define AVG_FILTER 1

#define USERSPACE_270_ROTATE 0

#define RECV_BUF_SIZE 1540
#define LIFTOFF_TIMEOUT 25000

#define MAX_TOUCH 10
#define MAX_CLIST 75
#define MAX_DELTA 25 // this value is squared to prevent the need to use sqrt

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
	float i;
	float j;
    unsigned short isValid;
};

int tpcmp(const void *v1, const void *v2)
{
    return ((*(struct candidate *)v2).pw - (*(struct candidate *)v1).pw);
}
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define isBetween(A, B, C) ( ((A-B) > 0) && ((A-C) < 0) )

int dist(int x1, int y1, int x2, int y2)  {
	return pow(x1 - x2, 2)+pow(y1 - y2, 2);
}

#if WANT_MULTITOUCH
struct touchpoint tpoint[MAX_TOUCH];
struct touchpoint prevtpoint[MAX_TOUCH];
struct touchpoint prev2tpoint[MAX_TOUCH];

#if AVG_FILTER
int isClose(struct touchpoint a, struct touchpoint b)
{
	if (isBetween(b.i, a.i+2.5, a.i-2.5) && isBetween(b.j, a.j+2.5, a.j-2.5))
		return 1;
	return 0;
}

//return 1 if b is closer
//return 2 if c is closer
int find_closest(struct touchpoint a, struct touchpoint b, struct touchpoint c)
{
	int diffB = fabs(a.i - b.i) + fabs(a.j - b.j);
	int diffC = fabs(a.i - c.i) + fabs(a.j - c.j);

	if (diffB < diffC)
		return 1;
	else
		return 2;
}

int avg_filter(struct touchpoint *t) {
	int tp1_found, tp2_found, i;
	tp1_found = tp2_found = -1;

	for(i=0; i<MAX_TOUCH; i++) {
		if(isClose(*t, prevtpoint[i])) {
			if(tp1_found < 0) {
				tp1_found = i;
			} else {
				if (find_closest(*t, prevtpoint[tp1_found], prevtpoint[i]) == 2)
					tp1_found = i;
			}
		}
		if(isClose(*t, prev2tpoint[i])) {
			if(tp2_found < 0) {
				tp2_found = i;
			} else {
                if (find_closest(*t, prev2tpoint[tp2_found], prev2tpoint[i]) == 2)
                    tp2_found = i;
			}
		}
	}
#if DEBUG
	printf("before: i=%f, j=%f", t->i, t->j);
#endif 
	if (tp1_found >= 0 && tp2_found >= 0) {
		t->i = (t->i + prevtpoint[tp1_found].i + prev2tpoint[tp2_found].i) / 3.0;
		t->j = (t->j + prevtpoint[tp1_found].j + prev2tpoint[tp2_found].j) / 3.0;
	}
#if DEBUG
    printf("|||| after: i=%f, j=%f\n", t->i, t->j);
#endif
	return 0;
}
#endif //AVG_FILTER

int calc_point()
{
	int i,j;
	int tweight=0;
	int tpc=0;
	float isum=0, jsum=0;
	float avgi, avgj;
	float powered;
	static int previoustpc;
	int clc=0;
	struct candidate clist[MAX_CLIST];

    //Record values for processing later
	for(i=0; i < MAX_TOUCH; i++) {
		prev2tpoint[i].i = prevtpoint[i].i;
		prev2tpoint[i].j = prevtpoint[i].j;
		prev2tpoint[i].pw = prevtpoint[i].pw;
		prevtpoint[i].i = tpoint[i].i;
		prevtpoint[i].j = tpoint[i].j;
		prevtpoint[i].pw = tpoint[i].pw;
	}

	// generate list of high values
	for(i=0; i < 30; i++) {
		for(j=0; j < 40; j++) {
#if RAW_DATA_DEBUG
			printf("%2.2X ", matrix[i][j]);
#endif
			if(matrix[i][j] > 32 && clc < MAX_CLIST) {
				int cvalid=1;
				clist[clc].pw = matrix[i][j];
				clist[clc].i = i;
				clist[clc].j = j;
				//check if local maxima
				if(i>0  && matrix[i-1][j] > matrix[i][j]) cvalid = 0;
				if(i<29 && matrix[i+1][j] > matrix[i][j]) cvalid = 0;
				if(j>0  && matrix[i][j-1] > matrix[i][j]) cvalid = 0;
				if(j<39 && matrix[i][j+1] > matrix[i][j]) cvalid = 0; 
				if(cvalid) clc++;
			}
		}
#if RAW_DATA_DEBUG
		printf("\n");
#endif
	}
#if DEBUG
	printf("%d clc\n", clc);
#endif

	// sort candidate list by strength
	//qsort(clist, clc, sizeof(clist[0]), tpcmp);

#if DEBUG
	printf("%d %d %d \n", clist[0].pw, clist[1].pw, clist[2].pw);
#endif

	int k, l;
	for(k=0; k < MIN(clc, 20); k++) {
		int newtp=1;

		int rad=3; // radius around candidate to use for calculation
		int mini = clist[k].i - rad+1;
		int maxi = clist[k].i + rad;
		int minj = clist[k].j - rad+1;
		int maxj = clist[k].j + rad;
		
		// discard points close to already detected touches
		for(l=0; l<tpc; l++) {
			if(tpoint[l].i >= mini+1 && tpoint[l].i < maxi-1 && tpoint[l].j >= minj+1 && tpoint[l].j < maxj-1)
                newtp=0;
		}
		
		// calculate new touch near the found candidate
		if(newtp && tpc < MAX_TOUCH) {
			tweight=0;
			isum=0;
			jsum=0;
			for(i=MAX(0, mini); i < MIN(30, maxi); i++) {
				for(j=MAX(0, minj); j < MIN(40, maxj); j++) {
					int dd = dist(i,j,clist[k].i,clist[k].j);
					powered = matrix[i][j];
					if(dd == 2 && 0.65f * matrix[clist[k].i][clist[k].j] < matrix[i][j] ) 
						powered = 0.65f * matrix[clist[k].i][clist[k].j];
					if(dd == 4 && 0.15f * matrix[clist[k].i][clist[k].j] < matrix[i][j] ) 
						powered = 0.15f * matrix[clist[k].i][clist[k].j];
					if(dd == 5 && 0.10f * matrix[clist[k].i][clist[k].j] < matrix[i][j] ) 
						powered = 0.10f * matrix[clist[k].i][clist[k].j];
					if(dd == 8 && 0.05f * matrix[clist[k].i][clist[k].j] < matrix[i][j] ) 
						powered = 0.05f * matrix[clist[k].i][clist[k].j];
					
					powered = pow(powered, 1.5);
					tweight += powered;
					isum += powered * i;
					jsum += powered * j;
				}
			}
			avgi = isum / (float)tweight;
			avgj = jsum / (float)tweight;
			tpoint[tpc].pw = tweight;
			tpoint[tpc].i = avgi;
			tpoint[tpc].j = avgj;
			tpoint[tpc].isValid = 1;
			tpc++;
#if DEBUG
			printf("Coords %d %lf, %lf, %d\n", tpc, avgi, avgj, tweight);
#endif

		}
	}

	/* filter touches for impossibly large moves that indicate a liftoff and
	 * re-touch */
	for (k = 0; k < tpc; k++) {
		if (tpoint[k].isValid) {
			float smallest_delta = 1000; // set to impossibly large number
			int smallest_delta_location;
			for (l = 0; l < tpc; l++) {
				if (tpoint[l].isValid) {
					float deltai, deltaj, total_delta;
					deltai = tpoint[k].i - prevtpoint[l].i;
					deltaj = tpoint[k].j - prevtpoint[l].j;
					total_delta = (deltai * deltai) + (deltaj * deltaj); // calculate hypotenuse
					if (total_delta < smallest_delta) {
						smallest_delta = total_delta;
						smallest_delta_location = l;
					}
				}
			}
			if (smallest_delta > MAX_DELTA && previoustpc == 1 && tpc == 1) {
				/* Previous code did not send any liftoffs unless all touches
				 * were lifted. For now we send a liftoff without a tracking
				 * ID when there are only 1 touch involved */
				send_uevent(uinput_fd, EV_KEY, BTN_TOUCH, 0);
				send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
				send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
#if DEBUG
				printf("smallest_delta %lf\n", smallest_delta);
#endif
			}
		}
	}

	//report touches
	for (k = 0; k < tpc; k++) {
		if (tpoint[k].isValid) {
#if AVG_FILTER
			avg_filter(&tpoint[k]);
#endif //AVG_FILTER
			send_uevent(uinput_fd, EV_KEY, BTN_TOUCH, 1);
#if USERSPACE_270_ROTATE
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_X, tpoint[k].i*768/29);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, 1024-tpoint[k].j*1024/39);
#else
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, 768-tpoint[k].i*768/29);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_X, 1024-tpoint[k].j*1024/39);
#endif //USERSPACE_270_ROTATE
			send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
			tpoint[k].isValid = 0;
        }
    }
	send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
	previoustpc = tpc; // store the touch count for the next run
	return tpc; // return the touch count
}
#endif

#if WANT_SINGLETOUCH
void calc_point()
{
	int i,j;
	int tweight=0;
	float xsum=0, ysum=0;
	float avgx, avgy;
	float powered;

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
	avgx = xsum / (float)tweight;
	avgy = ysum / (float)tweight;

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

int cline_valid(unsigned int extras);
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

int cline_valid(unsigned int extras)
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

int consume_line()
{
	int i,j,ret=0;

	if(cline[1] == 0x47)
	{
		//calculate the data points. all transfers complete
		ret = calc_point();
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

	return ret;
}

int snarf2(unsigned char* bytes, int size)
{
	int i=0,ret=0;
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
		ret = consume_line();
	}

	if(cline_valid(0))
	{
		ret = consume_line();
//		printf("was valid2\n");
	}
	return ret;
	/* It's possible that this function might return a 0 when previously there
	 * was a valid touch count, but in practice this doesn't appear to happen */
}

void open_uinput()
{
    struct uinput_user_dev device;

    memset(&device, 0, sizeof device);

    uinput_fd=open(UINPUT_LOCATION,O_WRONLY);
    strcpy(device.name,"HPTouchpad");

    device.id.bustype=BUS_VIRTUAL;
    device.id.vendor=1;
    device.id.product=1;
    device.id.version=1;

#if WANT_SINGLETOUCH
	int i;
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
    device.absmax[ABS_PRESSURE]=255;
    device.absfuzz[ABS_PRESSURE]=0;
    device.absflat[ABS_PRESSURE]=0;
#endif
#if WANT_MULTITOUCH
#if USERSPACE_270_ROTATE
    device.absmax[ABS_MT_POSITION_X]=768;
    device.absmax[ABS_MT_POSITION_Y]=1024;
#else
    device.absmax[ABS_MT_POSITION_X]=1024;
    device.absmax[ABS_MT_POSITION_Y]=768;
#endif // USERSPACE_270_ROTATE
    device.absmin[ABS_MT_POSITION_X]=0;
    device.absmin[ABS_MT_POSITION_Y]=0;
    device.absfuzz[ABS_MT_POSITION_X]=2;
    device.absflat[ABS_MT_POSITION_X]=0;
    device.absfuzz[ABS_MT_POSITION_Y]=1;
    device.absflat[ABS_MT_POSITION_Y]=0;
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

#if WANT_SINGLETOUCH
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
/*    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_TOUCH_MAJOR) < 0)
            fprintf(stderr, "error tool rel\n");

    if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_WIDTH_MAJOR) < 0)
            fprintf(stderr, "error tool rel\n");
*/
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

void liftoff()
{
	// clears arrays and sends liftoff events - nothing is touching the screen
	int i;
	for(i=0; i<MAX_TOUCH; i++) {
		tpoint[i].i = -10;
		tpoint[i].j = -10;
		prevtpoint[i].i = -10;
		prevtpoint[i].j = -10;
		prev2tpoint[i].i = -10;
		prev2tpoint[i].j = -10;
	}
	send_uevent(uinput_fd, EV_KEY, BTN_TOUCH, 0);
	send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
	send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

int main(int argc, char** argv)
{
	struct hsuart_mode uart_mode;
	int uart_fd, nbytes;
	unsigned char recv_buf[RECV_BUF_SIZE];
	fd_set fdset;
	struct timeval seltmout;
	struct sched_param sparam = { .sched_priority = 99 /* linux maximum, nonportable */};

	/* We set ts server priority to RT so that there is no delay in
	 * in obtaining input and we are NEVER bumped from CPU until we
	 * give it up ourselves. */
	if (sched_setscheduler(0 /* that's us */, SCHED_FIFO, &sparam))
		perror("Cannot set RT priority, ignoring: ");
	
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

	ioctl(uart_fd, HSUART_IOCTL_FLUSH, 0x9);

	while(1)
	{
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

			liftoff();

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
#if DEBUG
		printf("Received %d bytes\n", nbytes);
		int i;
		for(i=0; i < nbytes; i++)
			printf("%2.2X ",recv_buf[i]);
		printf("\n");
#endif
		if (!snarf2(recv_buf,nbytes))
			liftoff(); // sometimes there is data, but no valid touches
	}

	return 0;
}
