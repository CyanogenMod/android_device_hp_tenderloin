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
 * The code was written from scratch, the hard math and understanding the
 * device output by jonpry @ gmail
 * uinput bits and the rest by Oleg Drokin green@linuxhacker.ru
 * Multitouch detection by Rafael Brune mail@rbrune.de
 * Max delta, debounce, hover debounce, settings, and socket code by
 * Dees_Troy - dees_troy at yahoo
 * Tracking ID code by Dees_Troy and jyxent (Jordan Patterson)
 *
 * Copyright (c) 2011 CyanogenMod Touchpad Project.
 *
 *
 */

#define LOG_TAG "ts_srv"
#include <cutils/log.h>
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
#include <sys/socket.h>
#include <sys/un.h>

#include "digitizer.h"

#if 1
// This is for Android
#define UINPUT_LOCATION "/dev/uinput"
#else
// This is for webos and possibly other Linuxes
#define UINPUT_LOCATION "/dev/input/uinput"
#endif

#define TS_SOCKET_LOCATION "/dev/socket/tsdriver"
// Set to 1 to enable socket debug information
#define DEBUG_SOCKET 1

#define TS_SETTINGS_FILE "/data/tssettings"
// Set to 1 to enable settings file debug information
#define TS_SETTINGS_DEBUG 0

/* Set to 1 to print coordinates to stdout. */
#define DEBUG 0

/* Set to 1 to see raw data from the driver */
#define RAW_DATA_DEBUG 0
// Removes values below threshold for easy reading, set to 0 to see everything.
// A value of 2 should remove most unwanted output
#define RAW_DATA_THRESHOLD 0

// Set to 1 to see event logging
#define EVENT_DEBUG 0
// Set to 1 to enable tracking ID logging
#define TRACK_ID_DEBUG 0

#define AVG_FILTER 1

#define USERSPACE_270_ROTATE 0

#define RECV_BUF_SIZE 1540
#define LIFTOFF_TIMEOUT 25000
#define SOCKET_BUFFER_SIZE 10

#define MAX_TOUCH 10 // Max touches that will be reported

#define MAX_DELTA_FILTER 1 // Set to 1 to use max delta filtering
// This value determines when a large distance change between one touch
// and another will be reported as 2 separate touches instead of a swipe.
// This distance is in pixels.
#define MAX_DELTA 130
// If we exceed MAX_DELTA, we'll check the previous touch point to see if
// it was moving fairly far.  If the previous touch moved far enough and is
// within the same direction / angle, we'll allow it to be a swipe.
// This is the distance theshold that the previous touch must have traveled.
// This value is in pixels.
#define MIN_PREV_DELTA 40
// This is the angle, plus or minus that the previous direction must have
// been traveling.  This angle is an arctangent. (atan2)
#define MAX_DELTA_ANGLE 0.25
#define MAX_DELTA_DEBUG 0 // Set to 1 to see debug logging for max delta

// Any touch above this threshold is immediately reported to the system
#define TOUCH_INITIAL_THRESHOLD 32
int touch_initial_thresh = TOUCH_INITIAL_THRESHOLD;
// Previous touches that have already been reported will continue to be
// reported so long as they stay above this threshold
#define TOUCH_CONTINUE_THRESHOLD 26
int touch_continue_thresh = TOUCH_CONTINUE_THRESHOLD;
// New touches above this threshold but below TOUCH_INITIAL_THRESHOLD will not
// be reported unless the touch continues to appear.  This is designed to
// filter out brief, low threshold touches that may not be valid.
#define TOUCH_DELAY_THRESHOLD 28
int touch_delay_thresh = TOUCH_DELAY_THRESHOLD;
// Delay before a touch above TOUCH_DELAY_THRESHOLD but below
// TOUCH_INITIAL_THRESHOLD will be reported.  We will wait and see if this
// touch continues to show up in future buffers before reporting the event.
#define TOUCH_DELAY 5
int touch_delay_count = TOUCH_DELAY;
// Threshold for end of a large area. This value needs to be set low enough
// to filter out large touch areas and tends to be related to other touch
// thresholds.
#define LARGE_AREA_UNPRESS 22 //TOUCH_CONTINUE_THRESHOLD
#define LARGE_AREA_FRINGE 5 // Threshold for large area fringe

// These are stylus thresholds:
#define TOUCH_INITIAL_THRESHOLD_S  32
#define TOUCH_CONTINUE_THRESHOLD_S 16
#define TOUCH_DELAY_THRESHOLD_S    24
#define TOUCH_DELAY_S               2

// Enables filtering of a single touch to make it easier to long press.
// Keeps the initial touch point the same so long as it stays within
// the radius (note it's not really a radius and is actually a square)
#define DEBOUNCE_FILTER 1 // Set to 1 to enable the debouce filter
#define DEBOUNCE_RADIUS 10 // Radius for debounce in pixels
#define DEBOUNCE_DEBUG 0 // Set to 1 to enable debounce logging

// Enables filtering after swiping to prevent the slight jitter that
// sometimes happens while holding your finger still.  The radius is
// really a square. We don't start debouncing a hover unless the touch point
// stays within the radius for the number of cycles defined by
// HOVER_DEBOUNCE_DELAY
#define HOVER_DEBOUNCE_FILTER 1 // Set to 1 to enable hover debounce
#define HOVER_DEBOUNCE_RADIUS 2 // Radius for hover debounce in pixels
#define HOVER_DEBOUNCE_DELAY 30 // Count of delay before we start debouncing
#define HOVER_DEBOUNCE_DEBUG 0 // Set to 1 to enable hover debounce logging

// This is used to help calculate ABS_TOUCH_MAJOR
// This is roughly the value of 1024 / 40 or 768 / 30
#define PIXELS_PER_POINT 25

// This enables slots for the type B multi-touch protocol.
// The kernel must support slots (ABS_MT_SLOT). The TouchPad 2.6.35 kernel
// doesn't seem to handle liftoffs with protocol B properly so leave it off
// for now.
#define USE_B_PROTOCOL 0

/** ------- end of user modifiable parameters ---- */
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define isBetween(A, B, C) ( ((A-B) > 0) && ((A-C) < 0) )
// We square MAX_DELTA to prevent the need to use sqrt
#define MAX_DELTA_SQ (MAX_DELTA * MAX_DELTA)
#define MIN_PREV_DELTA_SQ (MIN_PREV_DELTA * MIN_PREV_DELTA)

#define X_AXIS_POINTS  30
#define Y_AXIS_POINTS  40
#define X_AXIS_MINUS1 X_AXIS_POINTS - 1 // 29
#define Y_AXIS_MINUS1 Y_AXIS_POINTS - 1 // 39

#if USERSPACE_270_ROTATE
#define X_RESOLUTION  768
#define Y_RESOLUTION 1024
#define X_LOCATION_VALUE ((float)X_RESOLUTION) / ((float)X_AXIS_MINUS1)
#define Y_LOCATION_VALUE ((float)Y_RESOLUTION) / ((float)Y_AXIS_MINUS1)
#else
#define X_RESOLUTION 1024
#define Y_RESOLUTION  768
#define X_LOCATION_VALUE ((float)X_RESOLUTION) / ((float)Y_AXIS_MINUS1)
#define Y_LOCATION_VALUE ((float)Y_RESOLUTION) / ((float)X_AXIS_MINUS1)
#endif // USERSPACE_270_ROTATE

#define X_RESOLUTION_MINUS1 X_RESOLUTION - 1
#define Y_RESOLUTION_MINUS1 Y_RESOLUTION - 1

struct touchpoint {
	// Power or weight of the touch, used for calculating the center point.
	int pw;
	// These store the average of the locations in the digitizer matrix that
	// make up the touch.  Used for calculating the center point.
	float i;
	float j;
#if USE_B_PROTOCOL
	// Slot used for the B protocol touch events.
	int slot;
#endif
	// Tracking ID that is assigned to this touch.
	int tracking_id;
	// Index location of this touch in the previous set of touches.
	int prev_loc;
#if MAX_DELTA_FILTER
	// Direction and distance between this touch and the previous touch.
	float direction;
	int distance;
#endif
	// Size of the touch area.
	int touch_major;
	// X and Y locations of the touch.  These values may have been changed by a
	// filter.
	int x;
	int y;
	// Unfiltered location of the touch.
	int unfiltered_x;
	int unfiltered_y;
	// The highest value found in the digitizer matrix of this touch area.
	int highest_val;
	// Delay count for touches that do not have a very high highest_val.
	int touch_delay;
#if HOVER_DEBOUNCE_FILTER
	// Location that we are tracking for hover debounce
	int hover_x;
	int hover_y;
	int hover_delay;
#endif
};

// This array contains the current touches (tpoint), previous touches
// (prevtpoint) and the touches from 2 times ago (prev2tpoint)
struct touchpoint tp[3][MAX_TOUCH];
// These indexes locate the appropriate set of touches in tp
int tpoint, prevtpoint, prev2tpoint;

// Used for reading data from the digitizer
unsigned char cline[64];
// Index used for cline
unsigned int cidx = 0;
// Contains all of the data from the digitizer
unsigned char matrix[X_AXIS_POINTS][Y_AXIS_POINTS];
// Indicates if a point in the digitizer matrix has already been scanned.
int invalid_matrix[X_AXIS_POINTS][Y_AXIS_POINTS];
// File descriptor for uinput device
int uinput_fd;
#if USE_B_PROTOCOL
// Indicates which slots are in use
int slot_in_use[MAX_TOUCH];
#endif

int send_uevent(int fd, __u16 type, __u16 code, __s32 value)
{
	struct input_event event;

#if EVENT_DEBUG
	char ctype[20], ccode[20];
	switch (type) {
		case EV_ABS:
			strcpy(ctype, "EV_ABS");
			break;
		case EV_KEY:
			strcpy(ctype, "EV_KEY");
			break;
		case EV_SYN:
			strcpy(ctype, "EV_SYN");
			break;
	}
	switch (code) {
		case ABS_MT_SLOT:
			strcpy(ccode, "ABS_MT_SLOT");
			break;
		case ABS_MT_TRACKING_ID:
			strcpy(ccode, "ABS_MT_TRACKING_ID");
			break;
		case ABS_MT_TOUCH_MAJOR:
			strcpy(ccode, "ABS_MT_TOUCH_MAJOR");
			break;
		case ABS_MT_POSITION_X:
			strcpy(ccode, "ABS_MT_POSITION_X");
			break;
		case ABS_MT_POSITION_Y:
			strcpy(ccode, "ABS_MT_POSITION_Y");
			break;
		case SYN_MT_REPORT:
			strcpy(ccode, "SYN_MT_REPORT");
			break;
		case SYN_REPORT:
			strcpy(ccode, "SYN_REPORT");
			break;
		case BTN_TOUCH:
			strcpy(ccode, "BTN_TOUCH");
			break;
	}
	ALOGI("event type: '%s' code: '%s' value: %i \n", ctype, ccode, value);
#endif

	memset(&event, 0, sizeof(event));
	event.type = type;
	event.code = code;
	event.value = value;

	if (write(fd, &event, sizeof(event)) != sizeof(event)) {
		ALOGE("Error on send_event %d", sizeof(event));
		return -1;
	}

	return 0;
}

#if AVG_FILTER
void avg_filter(struct touchpoint *t) {
#if DEBUG
	ALOGD("before: x=%d, y=%d", t->x, t->y);
#endif
	float total_div = 6.0;
	int xsum = 4 * t->unfiltered_x + 2 *
		tp[prevtpoint][t->prev_loc].unfiltered_x;
	int ysum = 4 * t->unfiltered_y + 2 *
		tp[prevtpoint][t->prev_loc].unfiltered_y;

	if(tp[prevtpoint][t->prev_loc].prev_loc > -1) {
		xsum +=
			tp[prev2tpoint][tp[prevtpoint][t->prev_loc].prev_loc].unfiltered_x;
		ysum +=
			tp[prev2tpoint][tp[prevtpoint][t->prev_loc].prev_loc].unfiltered_y;
		total_div += 1.0;
	}
	t->x = xsum / total_div;
	t->y = ysum / total_div;
#if DEBUG
	ALOGD("|||| after: x=%d, y=%d\n", t->x, t->y);
#endif
}
#endif // AVG_FILTER

#if HOVER_DEBOUNCE_FILTER
void hover_debounce(int i) {
	int prev_loc = tp[tpoint][i].prev_loc;

	tp[tpoint][i].hover_delay = tp[prevtpoint][prev_loc].hover_delay;
	// Check to see if the current touch, previous touch, and prev2 touch are
	// all within the HOVER_DEBOUNCE_RADIUS
	if (abs(tp[tpoint][i].x - tp[prevtpoint][prev_loc].hover_x) <
		HOVER_DEBOUNCE_RADIUS &&
		abs(tp[tpoint][i].y - tp[prevtpoint][prev_loc].hover_y) <
		HOVER_DEBOUNCE_RADIUS) {
		if (!tp[tpoint][i].hover_delay) {
			tp[tpoint][i].x = tp[prevtpoint][prev_loc].hover_x;
			tp[tpoint][i].y = tp[prevtpoint][prev_loc].hover_y;
#if HOVER_DEBOUNCE_DEBUG
			ALOGD("Debouncing tracking ID: %i\n", tp[tpoint][i].tracking_id);
#endif
		} else {
			// We're still within the radius but haven't been in the radius
			// long enough.
			tp[tpoint][i].hover_delay--;
#if HOVER_DEBOUNCE_DEBUG
			ALOGD("Hover delay of %i on tracking ID: %i\n",
				tp[tpoint][i].hover_delay, tp[tpoint][i].tracking_id);
#endif
		}
		if (tp[prevtpoint][prev_loc].hover_delay == 1) {
			// Do not bring forward the hover points... we will hover from
			// here.  This prevents some jerking backwards as we switch between
			// hovering and not hovering.
		} else {
			// Bring forward the hover points from previous location.
			tp[tpoint][i].hover_x = tp[prevtpoint][prev_loc].hover_x;
			tp[tpoint][i].hover_y = tp[prevtpoint][prev_loc].hover_y;
		}
	} else {
		// We have moved too far for hover debouce, reset the delay counter.
		tp[tpoint][i].hover_delay = HOVER_DEBOUNCE_DELAY;
	}
}
#endif // HOVER_DEBOUNCE_FILTER

#if USE_B_PROTOCOL
void liftoff_slot(int slot) {
	// Sends a liftoff indicator for a specific slot
#if EVENT_DEBUG
	ALOGD("liftoff slot function, lifting off slot: %i\n", slot);
#endif
	// According to the Linux kernel documentation, this is the right events
	// to send for protocol B, but the TouchPad 2.6.35 kernel doesn't seem to
	// handle them correctly.
	send_uevent(uinput_fd, EV_ABS, ABS_MT_SLOT, slot);
	send_uevent(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
}
#endif // USE_B_PROTOCOL

void liftoff(void)
{
#if USE_B_PROTOCOL
	// Send liftoffs for any slots that haven't been lifted off
	int i;
	for (i=0; i<MAX_TOUCH; i++) {
		if (slot_in_use[i]) {
			slot_in_use[i] = 0;
			liftoff_slot(i);
		}
	}
#endif
	// Sends liftoff events - nothing is touching the screen
#if EVENT_DEBUG
	ALOGD("liftoff function\n");
#endif
#if !USE_B_PROTOCOL
	send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
#endif
	send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

void determine_area_loc_fringe(float *isum, float *jsum, int *tweight, int i,
	int j, int cur_touch_id){
	float powered;

	// Set fringe point to used for this touch point
	invalid_matrix[i][j] = cur_touch_id;

	// Track touch values to help determine the pixel x, y location
	powered = pow(matrix[i][j], 1.5);
	*tweight += powered;
	*isum += powered * i;
	*jsum += powered * j;

	// Check the nearby points to see if they are above LARGE_AREA_FRINGE
	// but still decreasing in value to ensure that they are part of the same
	// touch and not a nearby, pinching finger.
	if (i > 0 && invalid_matrix[i-1][j] != cur_touch_id)
	{
		if (matrix[i-1][j] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j,
				cur_touch_id);
	}
	if (i < X_AXIS_MINUS1 && invalid_matrix[i+1][j] != cur_touch_id)
	{
		if (matrix[i+1][j] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j,
				cur_touch_id);
	}
	if (j > 0 && invalid_matrix[i][j-1] != cur_touch_id) {
		if (matrix[i][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i, j - 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && invalid_matrix[i][j+1] != cur_touch_id)
	{
		if (matrix[i][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i, j + 1,
				cur_touch_id);
	}
	if (i > 0 && j > 0 && invalid_matrix[i-1][j-1] != cur_touch_id)
	{
		if (matrix[i-1][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j - 1,
				cur_touch_id);
	}
	if (i < X_AXIS_MINUS1 && j > 0 && invalid_matrix[i+1][j-1] != cur_touch_id)
	{
		if (matrix[i+1][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j - 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && i > 0 && invalid_matrix[i-1][j+1] != cur_touch_id)
	{
		if (matrix[i-1][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j + 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && i < X_AXIS_MINUS1 &&
		invalid_matrix[i+1][j+1] != cur_touch_id)
	{
		if (matrix[i+1][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j + 1,
				cur_touch_id);
	}
}

void determine_area_loc(float *isum, float *jsum, int *tweight, int i, int j,
	int *mini, int *maxi, int *minj, int *maxj, int cur_touch_id,
	int *highest_val){
	float powered;

	// Invalidate this touch point so that we don't process it later
	invalid_matrix[i][j] = cur_touch_id;

	// Track the size of the touch for TOUCH_MAJOR
	if (i < *mini)
		*mini = i;
	if (i > *maxi)
		*maxi = i;
	if (j < *minj)
		*minj = j;
	if (j > *maxj)
		*maxj = j;

	// Track the highest value of the touch to determine which threshold
	// applies.
	if (matrix[i][j] > *highest_val)
		*highest_val = matrix[i][j];

	// Track touch values to help determine the pixel x, y location
	powered = pow(matrix[i][j], 1.5);
	*tweight += powered;
	*isum += powered * i;
	*jsum += powered * j;

	// Check nearby points to see if they are above LARGE_AREA_UNPRESS
	// or if they are above LARGE_AREA_FRINGE but the next nearby point is
	// decreasing in value.  If the value is not decreasing and below
	// LARGE_AREA_UNPRESS then we have 2 fingers pinched close together.
	if (i > 0 && invalid_matrix[i-1][j] != cur_touch_id)
	{
		if (matrix[i-1][j] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i - 1, j, mini, maxi, minj,
			maxj, cur_touch_id, highest_val);
		else if (matrix[i-1][j] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j,
				cur_touch_id);
	}
	if (i < X_AXIS_MINUS1 && invalid_matrix[i+1][j] != cur_touch_id)
	{
		if (matrix[i+1][j] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i + 1, j, mini, maxi, minj,
			maxj, cur_touch_id, highest_val);
		else if (matrix[i+1][j] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j,
				cur_touch_id);
	}
	if (j > 0 && invalid_matrix[i][j-1] != cur_touch_id)
	{
		if (matrix[i][j-1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i, j - 1, mini, maxi, minj,
				maxj, cur_touch_id, highest_val);
		else if (matrix[i][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i, j - 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && invalid_matrix[i][j+1] != cur_touch_id)
	{
		if (matrix[i][j+1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i, j + 1, mini, maxi, minj,
				maxj, cur_touch_id, highest_val);
		else if (matrix[i][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i, j + 1,
				cur_touch_id);
	}
	if (i > 0 && j > 0 && invalid_matrix[i-1][j-1] != cur_touch_id)
	{
		if (matrix[i-1][j-1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i - 1, j - 1, mini, maxi,
				minj, maxj, cur_touch_id, highest_val);
		else if (matrix[i-1][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j - 1,
				cur_touch_id);
	}
	if (i < X_AXIS_MINUS1 && j > 0 && invalid_matrix[i+1][j-1] != cur_touch_id)
	{
		if (matrix[i+1][j-1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i + 1, j - 1, mini, maxi,
				minj, maxj, cur_touch_id, highest_val);
		else if (matrix[i+1][j-1] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j-1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j - 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && i > 0 && invalid_matrix[i-1][j+1] != cur_touch_id)
	{
		if (matrix[i-1][j+1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i - 1, j + 1, mini, maxi,
				minj, maxj, cur_touch_id, highest_val);
		else if (matrix[i-1][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i-1][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i - 1, j + 1,
				cur_touch_id);
	}
	if (j < Y_AXIS_MINUS1 && i < X_AXIS_MINUS1 &&
		invalid_matrix[i+1][j+1] != cur_touch_id)
	{
		if (matrix[i+1][j+1] >= LARGE_AREA_UNPRESS)
			determine_area_loc(isum, jsum, tweight, i + 1, j + 1, mini, maxi,
				minj, maxj, cur_touch_id, highest_val);
		else if (matrix[i+1][j+1] >= LARGE_AREA_FRINGE &&
			matrix[i+1][j+1] < matrix[i][j])
			determine_area_loc_fringe(isum, jsum, tweight, i + 1, j + 1,
				cur_touch_id);
	}
}

void process_new_tpoint(struct touchpoint *t, int *tracking_id) {
	// Handles setting up a brand new touch point
	if (t->highest_val > touch_delay_thresh) {
		t->tracking_id = *tracking_id;
		*tracking_id += 1;
		if (t->highest_val <= touch_initial_thresh)
			t->touch_delay = touch_delay_count;
	} else {
		t->highest_val = 0;
	}
}

int calc_point(void)
{
	int i, j, k;
	int tweight = 0;
	int tpc = 0;
	float isum = 0, jsum = 0;
	float avgi, avgj;
	static int previoustpc, tracking_id = 0;
#if DEBOUNCE_FILTER
	int new_debounce_touch = 0;
	static int initialx, initialy;
#endif

	if (tp[tpoint][0].x < -20) {
		// We had a total liftoff
		previoustpc = 0;
#if DEBOUNCE_FILTER
		new_debounce_touch = 1;
#endif
	} else {
		// Re-assign array indexes
		prev2tpoint = prevtpoint;
		prevtpoint = tpoint;
		tpoint++;
		if (tpoint > 2)
			tpoint = 0;
	}

	// Scan the digitizer data and generate a list of touches
	memset(&invalid_matrix, 0, sizeof(invalid_matrix));
	for(i=0; i < X_AXIS_POINTS; i++) {
		for(j=0; j < Y_AXIS_POINTS; j++) {
#if RAW_DATA_DEBUG
			if (matrix[i][j] < RAW_DATA_THRESHOLD)
				ALOGD("   ");
			else
				ALOGD("%2.2X ", matrix[i][j]);
#endif
			if (tpc < MAX_TOUCH && matrix[i][j] > touch_continue_thresh &&
				!invalid_matrix[i][j]) {

				isum = 0;
				jsum = 0;
				tweight = 0;
				int mini = i, maxi = i, minj = j, maxj = j;
				int highest_val = matrix[i][j];
				determine_area_loc(&isum, &jsum, &tweight, i, j, &mini,
					&maxi, &minj, &maxj, tpc + 1, &highest_val);

				avgi = isum / (float)tweight;
				avgj = jsum / (float)tweight;
				maxi = maxi - mini;
				maxj = maxj - minj;

				tp[tpoint][tpc].pw = tweight;
				tp[tpoint][tpc].i = avgi;
				tp[tpoint][tpc].j = avgj;
				tp[tpoint][tpc].touch_major = MAX(maxi, maxj) *
					PIXELS_PER_POINT;
				tp[tpoint][tpc].tracking_id = -1;
#if USE_B_PROTOCOL
				tp[tpoint][tpc].slot = -1;
#endif
				tp[tpoint][tpc].prev_loc = -1;
#if USERSPACE_270_ROTATE
				tp[tpoint][tpc].x = tp[tpoint][tpc].i * X_LOCATION_VALUE;
				tp[tpoint][tpc].y = Y_RESOLUTION_MINUS1 - tp[tpoint][tpc].j *
					Y_LOCATION_VALUE;
#else
				tp[tpoint][tpc].x = X_RESOLUTION_MINUS1 - tp[tpoint][tpc].j *
					X_LOCATION_VALUE;
				tp[tpoint][tpc].y = Y_RESOLUTION_MINUS1 - tp[tpoint][tpc].i *
					Y_LOCATION_VALUE;
#endif // USERSPACE_270_ROTATE
				// It is possible for x and y to be negative with the math
				// above so we force them to 0 if they are negative.
				if (tp[tpoint][tpc].x < 0)
					tp[tpoint][tpc].x = 0;
				if (tp[tpoint][tpc].y < 0)
					tp[tpoint][tpc].y = 0;
				tp[tpoint][tpc].unfiltered_x = tp[tpoint][tpc].x;
				tp[tpoint][tpc].unfiltered_y = tp[tpoint][tpc].y;
				tp[tpoint][tpc].highest_val = highest_val;
				tp[tpoint][tpc].touch_delay = 0;
#if HOVER_DEBOUNCE_FILTER
				tp[tpoint][tpc].hover_x = tp[tpoint][tpc].x;
				tp[tpoint][tpc].hover_y = tp[tpoint][tpc].y;
				tp[tpoint][tpc].hover_delay = HOVER_DEBOUNCE_DELAY;
#endif
				tpc++;
			}
		}
#if RAW_DATA_DEBUG
		ALOGD(" |\n"); // end of row
#endif
	}
#if RAW_DATA_DEBUG
	ALOGD("end of raw data\n"); // helps separate one frame from the next
#endif

#if USE_B_PROTOCOL
	// Set all previously used slots to -1 so we know if we need to lift any
	// of them off after matching
	for (i=0; i<MAX_TOUCH; i++)
		if(slot_in_use[i])
			slot_in_use[i] = -1;
#endif

	// Match up tracking IDs
	{
		int smallest_distance[MAX_TOUCH], cur_distance;
		int deltax, deltay;
		int smallest_distance_loc[MAX_TOUCH];
		// Find closest points for each touch
		for (i=0; i<tpc; i++) {
			smallest_distance[i] = 1000000;
			smallest_distance_loc[i] = -1;
			for (j=0; j<previoustpc; j++) {
				if (tp[prevtpoint][j].highest_val) {
					deltax = tp[tpoint][i].unfiltered_x -
						tp[prevtpoint][j].unfiltered_x;
					deltay = tp[tpoint][i].unfiltered_y -
						tp[prevtpoint][j].unfiltered_y;
					cur_distance = (deltax * deltax) + (deltay * deltay);
					if(cur_distance < smallest_distance[i]) {
						smallest_distance[i] = cur_distance;
						smallest_distance_loc[i] = j;
					}
				}
			}
		}

		// Remove mapping for touches which aren't closest
		for (i=0; i<tpc; i++) {
			for (j=i + 1; j<tpc; j++) {
				if (smallest_distance_loc[i] > -1 &&
				   smallest_distance_loc[i] == smallest_distance_loc[j]) {
					if (smallest_distance[i] < smallest_distance[j])
						smallest_distance_loc[j] = -1;
					else
						smallest_distance_loc[i] = -1;
				}
			}
		}

		// Assign ids to closest touches
		for (i=0; i<tpc; i++) {
			if (smallest_distance_loc[i] > -1) {
#if MAX_DELTA_FILTER
				// Filter for impossibly large changes in touches
				if (smallest_distance[i] > MAX_DELTA_SQ) {
					int need_lift = 1;
					// Check to see if the previous point was moving quickly
					if (tp[prevtpoint][smallest_distance_loc[i]].distance >
						MIN_PREV_DELTA_SQ) {
						// Check the direction of the previous point and see
						// if we're continuing in roughly the same direction.
						tp[tpoint][i].direction = atan2(
						tp[tpoint][i].x -
						tp[prevtpoint][smallest_distance_loc[i]].x,
						tp[tpoint][i].y -
						tp[prevtpoint][smallest_distance_loc[i]].y);
						if (fabsf(tp[tpoint][i].direction -
							tp[prevtpoint][smallest_distance_loc[i]].direction)
							< MAX_DELTA_ANGLE) {
#if MAX_DELTA_DEBUG
							ALOGD("direction is close enough, no liftoff\n");
#endif
							// No need to lift off
							need_lift = 0;
						}
#if MAX_DELTA_DEBUG
						else
							ALOGD("angle change too great, going to lift\n");
#endif
					}
#if MAX_DELTA_DEBUG
					else
						ALOGD("previous distance too low, going to lift\n");
#endif
					if (need_lift) {
						//  This is an impossibly large change in touches
#if TRACK_ID_DEBUG
						ALOGD("Over Delta %d - %d,%d - %d,%d -> %d,%d\n",
							tp[prevtpoint][smallest_distance_loc[i]].
							tracking_id,
							smallest_distance_loc[i], i, tp[tpoint][i].x,
							tp[tpoint][i].y,
							tp[prevtpoint][smallest_distance_loc[i]].x,
							tp[prevtpoint][smallest_distance_loc[i]].y);
#endif
#if USE_B_PROTOCOL
#if EVENT_DEBUG || MAX_DELTA_DEBUG
						ALOGD("sending max delta liftoff for slot: %i\n",
							tp[prevtpoint][smallest_distance_loc[i]].slot);
#endif // EVENT_DEBUG || MAX_DELTA_DEBUG
						liftoff_slot(
							tp[prevtpoint][smallest_distance_loc[i]].slot);
#endif // USE_B_PROTOCOL
						process_new_tpoint(&tp[tpoint][i], &tracking_id);
					}
				} else
#endif // MAX_DELTA_FILTER
				{
#if TRACK_ID_DEBUG
					ALOGD("Continue Map %d - %d,%d - %lf,%lf -> %lf,%lf\n",
						tp[prevtpoint][smallest_distance_loc[i]].tracking_id,
						smallest_distance_loc[i], i, tp[tpoint][i].i,
						tp[tpoint][i].j,
						tp[prevtpoint][smallest_distance_loc[i]].i,
						tp[prevtpoint][smallest_distance_loc[i]].j);
#endif
					tp[tpoint][i].tracking_id =
						tp[prevtpoint][smallest_distance_loc[i]].tracking_id;
					tp[tpoint][i].prev_loc = smallest_distance_loc[i];
					tp[tpoint][i].touch_delay =
						tp[prevtpoint][smallest_distance_loc[i]].touch_delay;
#if MAX_DELTA_FILTER
					// Track distance and angle
					tp[tpoint][i].distance = smallest_distance[i];
					tp[tpoint][i].direction = atan2(
						tp[tpoint][i].x -
						tp[prevtpoint][smallest_distance_loc[i]].x,
						tp[tpoint][i].y -
						tp[prevtpoint][smallest_distance_loc[i]].y);
#endif // MAX_DELTA_FILTER
#if AVG_FILTER
					avg_filter(&tp[tpoint][i]);
#endif // AVG_FILTER
#if HOVER_DEBOUNCE_FILTER
					hover_debounce(i);
#endif // HOVER_DEBOUNCE_FILTER
				}
#if USE_B_PROTOCOL
				tp[tpoint][i].slot =
					tp[prevtpoint][smallest_distance_loc[i]].slot;
				slot_in_use[tp[prevtpoint][smallest_distance_loc[i]].slot] = 1;
#endif
			} else {
				process_new_tpoint(&tp[tpoint][i], &tracking_id);
#if TRACK_ID_DEBUG
				ALOGD("New Mapping - %lf,%lf - tracking ID: %i\n",
					tp[tpoint][i].i, tp[tpoint][i].j,
					tp[tpoint][i].tracking_id);
#endif
			}
		}
	}

#if USE_B_PROTOCOL
	// Assign unused slots to touches that don't have a slot yet
	for (i=0; i<tpc; i++) {
		if (tp[tpoint][i].slot < 0 && tp[tpoint][i].highest_val &&
			!tp[tpoint][i].touch_delay) {
			for (j=0; j<MAX_TOUCH; j++) {
				if (slot_in_use[j] <= 0) {
					if (slot_in_use[j] == -1) {
#if EVENT_DEBUG
						ALOGD("lifting unused slot %i & reassigning it\n", j);
#endif
						liftoff_slot(j);
					}
					tp[tpoint][i].slot = j;
					slot_in_use[j] = 1;
#if TRACK_ID_DEBUG
					ALOGD("new slot [%i] trackID: %i slot: %i | %lf , %lf\n",
						i, tp[tpoint][i].tracking_id, tp[tpoint][i].slot,
						tp[tpoint][i].i, tp[tpoint][i].j);
#endif
					j = MAX_TOUCH;
				}
			}
		}
	}

	// Lift off any previously used slots that haven't been reassigned
	for (i=0; i<MAX_TOUCH; i++) {
		if (slot_in_use[i] == -1) {
#if EVENT_DEBUG
			ALOGD("lifting off slot %i - no longer in use\n", i);
#endif
			liftoff_slot(i);
			slot_in_use[i] = 0;
		}
	}
#endif // USE_B_PROTOCOL

#if DEBOUNCE_FILTER
	// The debounce filter only works on a single touch.
	// We record the initial touchdown point, calculate a radius in
	// pixels and re-center the point if we're still within the
	// radius.  Once we leave the radius, we invalidate so that we
	// don't debounce again even if we come back to the radius.
	if (tpc == 1) {
		if (new_debounce_touch) {
			// We record the initial location of a new touch
			initialx = tp[tpoint][0].x;
			initialy = tp[tpoint][0].y;
#if DEBOUNCE_DEBUG
			ALOGD("new touch recorded at %i, %i\n", initialx, initialy);
#endif
		} else if (initialx > -20) {
			// See if the current touch is still inside the debounce
			// radius
			if (abs(initialx - tp[tpoint][0].x) <= DEBOUNCE_RADIUS
				&& abs(initialy - tp[tpoint][0].y) <= DEBOUNCE_RADIUS) {
				// Set the point to the original point - debounce!
				tp[tpoint][0].x = initialx;
				tp[tpoint][0].y = initialy;
#if DEBOUNCE_DEBUG
				ALOGD("debouncing!!!\n");
#endif
			} else {
				initialx = -100; // Invalidate
#if DEBOUNCE_DEBUG
				ALOGD("done debouncing\n");
#endif
			}
		}
	}
#endif

	// Report touches
	for (k = 0; k < tpc; k++) {
		if (tp[tpoint][k].highest_val && !tp[tpoint][k].touch_delay) {
#if EVENT_DEBUG
			ALOGD("send event for tracking ID: %i\n",
				tp[tpoint][k].tracking_id);
#endif
#if USE_B_PROTOCOL
			send_uevent(uinput_fd, EV_ABS, ABS_MT_SLOT, tp[tpoint][k].slot);
#endif
			send_uevent(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID,
				tp[tpoint][k].tracking_id);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR,
				tp[tpoint][k].touch_major);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_X, tp[tpoint][k].x);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, tp[tpoint][k].y);
			send_uevent(uinput_fd, EV_ABS, ABS_MT_PRESSURE, tp[tpoint][k].pw);
#if !USE_B_PROTOCOL
			send_uevent(uinput_fd, EV_SYN, SYN_MT_REPORT, 0);
#endif
		} else if (tp[tpoint][k].touch_delay) {
			// This touch didn't meet the threshold so we don't report it yet
			tp[tpoint][k].touch_delay--;
		}
	}
	if (tpc > 0) {
		send_uevent(uinput_fd, EV_SYN, SYN_REPORT, 0);
	}
	previoustpc = tpc; // Store the touch count for the next run
	if (tracking_id >  2147483000)
		tracking_id = 0; // Reset tracking ID counter if it gets too big
	return tpc; // Return the touch count
}


int cline_valid(unsigned int extras)
{
	if (cline[0] == 0xff && cline[1] == 0x43 && cidx == 44-extras) {
		return 1;
	}
	if (cline[0] == 0xff && cline[1] == 0x47 && cidx > 4 &&
		cidx == (cline[2]+4-extras)) {
		return 1;
	}
	return 0;
}

void put_byte(unsigned char byte)
{
	if(cidx==0 && byte != 0xFF)
		return;

	// Sometimes a send is aborted by the touch screen. all we get is an out of
	// place 0xFF
	if(byte == 0xFF && !cline_valid(1))
		cidx = 0;

	cline[cidx++] = byte;
}

int consume_line(void)
{
	int i,j,ret=0;

	if(cline[1] == 0x47) {
		// Calculate the data points. all transfers complete
		ret = calc_point();
	}

	if(cline[1] == 0x43) {
		// This is a start event. clear the matrix
		if(cline[2] & 0x80) {
			for(i=0; i < X_AXIS_POINTS; i++)
				for(j=0; j < Y_AXIS_POINTS; j++)
					matrix[i][j] = 0;
		}

		// Write the line into the matrix
		for(i=0; i < Y_AXIS_POINTS; i++)
			matrix[cline[2] & 0x1F][i] = cline[i+3];
	}

	cidx = 0;

	return ret;
}

int snarf2(unsigned char* bytes, int size)
{
	int i,ret=0;

	for(i=0; i < size; i++) {
		put_byte(bytes[i]);
		if(cline_valid(0))
			ret += consume_line();
	}

	return ret;
}

void open_uinput(void)
{
	struct uinput_user_dev device;

	memset(&device, 0, sizeof device);

	uinput_fd=open(UINPUT_LOCATION,O_WRONLY);
	strcpy(device.name,"HPTouchpad");

	device.id.bustype=BUS_VIRTUAL;
	device.id.vendor = 1;
	device.id.product = 1;
	device.id.version = 1;

	device.absmax[ABS_MT_POSITION_X] = X_RESOLUTION;
	device.absmax[ABS_MT_POSITION_Y] = Y_RESOLUTION;
	device.absmin[ABS_MT_POSITION_X] = 0;
	device.absmin[ABS_MT_POSITION_Y] = 0;
	device.absfuzz[ABS_MT_POSITION_X] = 2;
	device.absflat[ABS_MT_POSITION_X] = 0;
	device.absfuzz[ABS_MT_POSITION_Y] = 1;
	device.absflat[ABS_MT_POSITION_Y] = 0;

	device.absmax[ABS_MT_PRESSURE] = 2000;
	device.absmin[ABS_MT_PRESSURE] = 250;
	device.absfuzz[ABS_MT_PRESSURE] = 0;
	device.absflat[ABS_MT_PRESSURE] = 0;

	if (write(uinput_fd,&device,sizeof(device)) != sizeof(device))
		ALOGE("error setup\n");

	if (ioctl(uinput_fd,UI_SET_EVBIT, EV_SYN) < 0)
		ALOGE("error evbit key\n");

	if (ioctl(uinput_fd,UI_SET_EVBIT,EV_ABS) < 0)
		ALOGE("error evbit rel\n");

#if USE_B_PROTOCOL
	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_SLOT) < 0)
		ALOGE("error slot rel\n");
#endif

	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_TRACKING_ID) < 0)
		ALOGE("error trkid rel\n");

	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_TOUCH_MAJOR) < 0)
		ALOGE("error tool rel\n");

	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_PRESSURE) < 0)
		ALOGE("error tool rel\n");

	//if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_WIDTH_MAJOR) < 0)
	//	ALOGE("error tool rel\n");

	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_POSITION_X) < 0)
		ALOGE("error tool rel\n");

	if (ioctl(uinput_fd,UI_SET_ABSBIT,ABS_MT_POSITION_Y) < 0)
		ALOGE("error tool rel\n");

	if (ioctl(uinput_fd,UI_DEV_CREATE) < 0)
		ALOGE("error create\n");
}

void clear_arrays(void)
{
	// Clears array (for after a total liftoff occurs)
	int i, j;
	for (i=0; i<3; i++) {
		for(j=0; j<MAX_TOUCH; j++) {
			tp[i][j].pw = -1000;
			tp[i][j].i = -1000;
			tp[i][j].j = -1000;
#if USE_B_PROTOCOL
			tp[i][j].slot = -1;
#endif
			tp[i][j].tracking_id = -1;
			tp[i][j].prev_loc = -1;
#if MAX_DELTA_FILTER
			tp[i][j].direction = 0;
			tp[i][j].distance = 0;
#endif
			tp[i][j].touch_major = 0;
			tp[i][j].x = -1000;
			tp[i][j].y = -1000;
			tp[i][j].unfiltered_x = -1000;
			tp[i][j].unfiltered_y = -1000;
			tp[i][j].highest_val = -1000;
			tp[i][j].touch_delay = -1000;
#if HOVER_DEBOUNCE_FILTER
			tp[i][j].hover_x = -1000;
			tp[i][j].hover_y = -1000;
			tp[i][j].hover_delay = HOVER_DEBOUNCE_DELAY;
#endif
		}
	}
}

void open_uart(int *uart_fd) {
	struct hsuart_mode uart_mode;
	*uart_fd = open("/dev/ctp_uart", O_RDONLY|O_NONBLOCK);
	if(*uart_fd <= 0) {
		ALOGE("Could not open uart\n");
		exit(0);
	}

	ioctl(*uart_fd, HSUART_IOCTL_GET_UARTMODE, &uart_mode);
	uart_mode.speed = 0x3D0900;
	ioctl(*uart_fd, HSUART_IOCTL_SET_UARTMODE, &uart_mode);

	ioctl(*uart_fd, HSUART_IOCTL_FLUSH, 0x9);
}

void create_ts_socket(int *socket_fd) {
	// Create / open socket for input
	*socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*socket_fd >= 0) {
		struct sockaddr_un unaddr;
		unaddr.sun_family = AF_UNIX;
		strcpy(unaddr.sun_path, TS_SOCKET_LOCATION);
		unlink(unaddr.sun_path);
		int len, bind_fd;
		len = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
		bind_fd = bind(*socket_fd, (struct sockaddr *)&unaddr, len);
		if (bind_fd >= 0) {
			int listen_fd;
			listen_fd = listen(*socket_fd, 3);
#if DEBUG_SOCKET
			if (listen_fd < 0)
				ALOGE("Error listening to socket\n");
#endif
		}
#if DEBUG_SOCKET
		else
			ALOGE("Error binding socket\n");
#endif
	}
#if DEBUG_SOCKET
	else
		ALOGE("Error creating socket\n");
#endif
}

void set_ts_mode(int mode){
	if (mode == 0) {
		// Finger mode
		touch_initial_thresh = TOUCH_INITIAL_THRESHOLD;
		touch_continue_thresh = TOUCH_CONTINUE_THRESHOLD;
		touch_delay_thresh = TOUCH_DELAY_THRESHOLD;
		touch_delay_count = TOUCH_DELAY;
	} else {
		// Stylus mode
		touch_initial_thresh = TOUCH_INITIAL_THRESHOLD_S;
		touch_continue_thresh = TOUCH_CONTINUE_THRESHOLD_S;
		touch_delay_thresh = TOUCH_DELAY_THRESHOLD_S;
		touch_delay_count = TOUCH_DELAY_S;
	}
}

int read_settings_file(void) {
	// Check for and read the settings file.
	// If the file isn't found, finger mode will be the default mode
	FILE *fp;
	int setting, ret_val = 0;

	fp = fopen(TS_SETTINGS_FILE, "r");
	if (fp == NULL) {
#if TS_SETTINGS_DEBUG
		ALOGE("Unable to fopen settings file for reading\n");
#endif
		set_ts_mode(0);
		return 0;
	}
	setting = fgetc(fp);
	if (setting == EOF) {
#if TS_SETTINGS_DEBUG
		ALOGD("fgetc == EOF: %i\n", setting);
#endif
		set_ts_mode(0);
		ret_val = 0;
	} else if (setting == 0) {
#if TS_SETTINGS_DEBUG
		ALOGD("setting is: %i so setting finger mode\n", setting);
#endif
		set_ts_mode(0);
		ret_val = 0;
	} else if (setting == 1) {
#if TS_SETTINGS_DEBUG
		ALOGD("setting is: %i so setting stylus mode\n", setting);
#endif
		set_ts_mode(1);
		ret_val = 1;
	}
	fclose(fp);
	return ret_val;
}

void write_settings_file(int setting) {
	// Write to the settings file.
	FILE *fp;
	fp = fopen(TS_SETTINGS_FILE, "w");
	if (fp == NULL) {
#if TS_SETTINGS_DEBUG
		ALOGE("Unable to fopen settings file for writing\n");
#endif
		return;
	}
	setting = fputc(setting, fp);
#if TS_SETTINGS_DEBUG
	if (setting == EOF)
		ALOGD("fputc == EOF: %i\n", setting);
	else
		ALOGD("Successfully wrote to setting %i to settings file\n", setting);
#endif
	fclose(fp);
}

void process_socket_buffer(char *buffer[], int buffer_len, int *uart_fd,
	int accept_fd) {
	// Processes data that is received from the socket
	// O = open uart
	// C = close uart
	// F = finger mode
	// S = stylus mode
	// M = return current mode
	int i, return_val, buf;

	for (i=0; i<buffer_len; i++) {
		buf = (int)*buffer;
		if (buf == 67 /* 'C' */ && *uart_fd >= 0) {
			return_val = close(*uart_fd);
			*uart_fd = -1;
#if DEBUG_SOCKET
			ALOGD("uart closed: %i\n", return_val);
#endif
			touchscreen_power(0);
		}
		if (buf == 79 /* 'O' */ && *uart_fd < 0) {
			open_uart(uart_fd);
			touchscreen_power(1);
#if DEBUG_SOCKET
			ALOGD("uart opened at %i\n", *uart_fd);
#endif
		}
		if (buf == 70 /* 'F' */) {
			set_ts_mode(0);
			write_settings_file(0);
#if DEBUG_SOCKET
			ALOGD("finger mode set\n");
#endif
		}
		if (buf == 83 /* 'S' */) {
			set_ts_mode(1);
			write_settings_file(1);
#if DEBUG_SOCKET
			ALOGD("stylus mode set\n");
#endif
		}
		if (buf == 77 /* 'M' */) {
			char current_mode[1];
			int send_ret;

			current_mode[0] = read_settings_file();
			send_ret = send(accept_fd, (char*)current_mode,
				sizeof(*current_mode), 0);
#if DEBUG_SOCKET
			if (send_ret <= 0)
				ALOGE("Unable to send data to socket\n");
			else
				ALOGD("Sent current mode of %i to socket\n",
					(int)current_mode[0]);
#endif
		}
		buffer++;
	}
}

int main(int argc, char** argv)
{
	int uart_fd, nbytes, need_liftoff = 0, sel_ret, socket_fd;
	unsigned char recv_buf[RECV_BUF_SIZE];
	fd_set fdset;
	struct timeval seltmout;
	/* linux maximum priority is 99, nonportable */
	struct sched_param sparam = { .sched_priority = 99 };

	/* We set ts server priority to RT so that there is no delay in
	 * in obtaining input and we are NEVER bumped from CPU until we
	 * give it up ourselves. */
	if (sched_setscheduler(0 /* that's us */, SCHED_FIFO, &sparam))
		perror("Cannot set RT priority, ignoring: ");

	open_uart(&uart_fd);
	init_digitizer_fd();
	touchscreen_power(1);


	open_uinput();

	read_settings_file();

	// Lift off in case of driver crash or in case the driver was shut off to
	// save power by closing the uart.
	liftoff();
	clear_arrays();

	create_ts_socket(&socket_fd);

	while(1) {
		FD_ZERO(&fdset);
		if (uart_fd >= 0)
			FD_SET(uart_fd, &fdset);
		if (socket_fd >= 0)
			FD_SET(socket_fd, &fdset);
		seltmout.tv_sec = 0;
		/* 2x tmout */
		seltmout.tv_usec = LIFTOFF_TIMEOUT;

		sel_ret = select(MAX(uart_fd, socket_fd) + 1, &fdset, NULL, NULL,
			&seltmout);
		if (sel_ret == 0) {
			/* Timeout means no more data and probably need to lift off */
#if DEBUG
			ALOGE("timeout! no data coming from uart\n");
#endif

			if (need_liftoff) {
#if EVENT_DEBUG
				ALOGD("timeout called liftoff\n");
#endif
				liftoff();
				clear_arrays();
				need_liftoff = 0;
			}

			FD_ZERO(&fdset);
			if (uart_fd >= 0)
				FD_SET(uart_fd, &fdset);
			if (socket_fd >= 0)
				FD_SET(socket_fd, &fdset);
			/* Now enter indefinite sleep until input appears */
			select(MAX(uart_fd, socket_fd) + 1, &fdset, NULL, NULL, NULL);
			/* In case we were wrongly woken up check the event
			 * count again */
			continue;
		}

		if (uart_fd >= 0 && FD_ISSET(uart_fd, &fdset)) {
			// This is touch data from the uart
			nbytes = read(uart_fd, recv_buf, RECV_BUF_SIZE);

			if(nbytes <= 0)
				continue;
#if DEBUG
			ALOGD("Received %d bytes\n", nbytes);
			int i;
			for(i=0; i < nbytes; i++)
				ALOGD("%2.2X ",recv_buf[i]);
			ALOGD("\n");
#endif
			if (!snarf2(recv_buf,nbytes)) {
				// Sometimes there's data but no valid touches due to threshold
				if (need_liftoff) {
#if EVENT_DEBUG
					ALOGD("snarf2 called liftoff\n");
#endif
					liftoff();
					clear_arrays();
					need_liftoff = 0;
				}
			} else
				need_liftoff = 1;
		}

		if (socket_fd >= 0 && FD_ISSET(socket_fd, &fdset)) {
			// This is data from the socket
			int accept_fd;
			accept_fd = accept(socket_fd, NULL, NULL);
			if (accept_fd >= 0) {
				char recv_str[SOCKET_BUFFER_SIZE];
				int recv_ret;

				recv_ret = recv(accept_fd, recv_str, SOCKET_BUFFER_SIZE, 0);

                                recv_str[recv_ret]=0; // add string terminator

				if (recv_ret > 0) {
#if DEBUG_SOCKET
					ALOGD("Socket received %i byte(s): '%s'\n", recv_ret,
						recv_str);
#endif
					process_socket_buffer((char **)&recv_str, recv_ret,
						&uart_fd, accept_fd);
				}
#if DEBUG_SOCKET
				else {
					if (recv_ret < 0)
						ALOGE("Receive error\n");
					else
						ALOGD("No actual data to receive\n");
				}
				close(accept_fd);
			} else {
				ALOGE("Accept failed\n");
#endif
			}
		}
	}

	return 0;
}
