/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 <kang@insecure.ws>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights"
#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/ioctl.h>

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

char const *const LCD_FILE = "/sys/class/leds/lcd-backlight/brightness";
char const *const LED_FILE_RIGHT = 
                        "/sys/class/leds/core_navi_right/brightness";
char const *const LED_FILE_LEFT = 
                        "/sys/class/leds/core_navi_left/brightness";


/** TS power stuff */
static int vdd_fd, xres_fd, wake_fd, i2c_fd, ts_state;

void touchscreen_power(int enable)
{
    struct i2c_rdwr_ioctl_data i2c_ioctl_data;
    struct i2c_msg i2c_msg;
    __u8 i2c_buf[16];
    int rc;

    if (enable) {
	int retry_count = 0;

try_again:
	/* Set reset so the chip immediatelly sees it */
        lseek(xres_fd, 0, SEEK_SET);
        rc = write(xres_fd, "1", 1);
	LOGE_IF(rc != 1, "TSpower, failed set xres");

	/* Then power on */
        lseek(vdd_fd, 0, SEEK_SET);
        rc = write(vdd_fd, "1", 1);
	LOGE_IF(rc != 1, "TSpower, failed to enable vdd");

	/* Sleep some more for the voltage to stabilize */
        usleep(50000);

        lseek(wake_fd, 0, SEEK_SET);
        rc = write(wake_fd, "1", 1);
	LOGE_IF(rc != 1, "TSpower, failed to assert wake");

        lseek(xres_fd, 0, SEEK_SET);
        rc = write(xres_fd, "0", 1);
	LOGE_IF(rc != 1, "TSpower, failed to reset xres");

        usleep(50000);

        lseek(wake_fd, 0, SEEK_SET);
        rc = write(wake_fd, "0", 1);
	LOGE_IF(rc != 1, "TSpower, failed to deassert wake");

        usleep(50000);

        i2c_ioctl_data.nmsgs = 1;
        i2c_ioctl_data.msgs = &i2c_msg;

        i2c_msg.addr = 0x67;
        i2c_msg.flags = 0;
        i2c_msg.buf = i2c_buf;

        i2c_msg.len = 2;
        i2c_buf[0] = 0x08; i2c_buf[1] = 0;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl1 failed %d errno %d\n", rc, errno);
	/* Ok, so the TS failed to wake, we need to retry a few times
         * before totally giving up */
	if ((rc != 1) && (retry_count++ < 3)) {
		lseek(vdd_fd, 0, SEEK_SET);
		rc = write(vdd_fd, "0", 1);
		usleep(10000);
		LOGE("TS wakeup retry #%d\n", retry_count);
		goto try_again;
	}

        i2c_msg.len = 6;
        i2c_buf[0] = 0x31; i2c_buf[1] = 0x01; i2c_buf[2] = 0x08;
        i2c_buf[3] = 0x0C; i2c_buf[4] = 0x0D; i2c_buf[5] = 0x0A;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl2 failed %d errno %d\n", rc, errno);

        i2c_msg.len = 2;
        i2c_buf[0] = 0x30; i2c_buf[1] = 0x0F;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl3 failed %d errno %d\n", rc, errno);

        i2c_buf[0] = 0x40; i2c_buf[1] = 0x02;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl4 failed %d errno %d\n", rc, errno);

        i2c_buf[0] = 0x41; i2c_buf[1] = 0x10;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl5 failed %d errno %d\n", rc, errno);

        i2c_buf[0] = 0x0A; i2c_buf[1] = 0x04;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl6 failed %d errno %d\n", rc, errno);

        i2c_buf[0] = 0x08; i2c_buf[1] = 0x03;
        rc = ioctl(i2c_fd,I2C_RDWR,&i2c_ioctl_data);
	LOGE_IF( rc != 1, "TSPower, ioctl7 failed %d errno %d\n", rc, errno);

        lseek(wake_fd, 0, SEEK_SET);
        rc = write(wake_fd, "1", 1);
	LOGE_IF(rc != 1, "TSpower, failed to assert wake again");
    } else {
        lseek(vdd_fd, 0, SEEK_SET);
        rc = write(vdd_fd, "0", 1);
	LOGE_IF(rc != 1, "TSpower, failed to disable vdd");
        /* XXX, should be correllated with LIFTOFF_TIMEOUT in ts driver */
        usleep(80000);
    }
}

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned;

	already_warned = 0;

	LOGV("write_int: path %s, value %d", path, value);
	fd = open(path, O_RDWR);

	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			LOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int rgb_to_brightness(struct light_state_t const *state)
{
	int color = state->color & 0x00ffffff;

	return ((77*((color>>16) & 0x00ff))
		+ (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8;
}

static int set_light_notifications(struct light_device_t* dev,
			struct light_state_t const* state)
{
	int brightness =  rgb_to_brightness(state);
	int v = 0;
	int ret = 0;
	pthread_mutex_lock(&g_lock);

	if (brightness+state->color == 0 || brightness > 100) {
		if (state->color & 0x00ffffff)
			v = 100;
	} else
		v = 0;

	LOGI("color %u fm %u status %u is lit %u brightness", state->color, state->flashMode, v, (state->color & 0x00ffffff), brightness);
	ret = write_int(LED_FILE_RIGHT, v);
    write_int(LED_FILE_LEFT, v);
	pthread_mutex_unlock(&g_lock);
	return ret;
}

static int set_light_backlight(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);

	pthread_mutex_lock(&g_lock);
	err = write_int(LCD_FILE, brightness);

	/* TS power magic hack */
	if (brightness > 0 && ts_state == 0) {
		LOGI("Enabling touch screen");
		ts_state = 1;
		touchscreen_power(1);
	} else if (brightness == 0 && ts_state == 1) {
		LOGI("Disabling touch screen");
		ts_state = 0;
		touchscreen_power(0);
	}

	pthread_mutex_unlock(&g_lock);
	return err;
}

static int close_lights(struct light_device_t *dev)
{
	LOGV("close_light is called");
	if (dev)
		free(dev);

    touchscreen_power(0);

	return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
						struct hw_device_t **device)
{
	int (*set_light)(struct light_device_t *dev,
		struct light_state_t const *state);

	LOGV("open_lights: open with %s", name);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
		set_light = set_light_notifications;
	else
		return -EINVAL;

	pthread_mutex_init(&g_lock, NULL);

	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t *)module;
	dev->common.close = (int (*)(struct hw_device_t *))close_lights;
	dev->set_light = set_light;

	*device = (struct hw_device_t *)dev;

	/* TS file descriptors. Ignore errors. */
	vdd_fd = open("/sys/devices/platform/cy8ctma395/vdd", O_WRONLY);
	LOGE_IF(vdd_fd < 0, "TScontrol: Cannot open vdd - %d", errno);
	xres_fd = open("/sys/devices/platform/cy8ctma395/xres", O_WRONLY);
	LOGE_IF(xres_fd < 0, "TScontrol: Cannot open xres - %d", errno);
	wake_fd = open("/sys/user_hw/pins/ctp/wake/level", O_WRONLY);
	LOGE_IF(wake_fd < 0, "TScontrol: Cannot open wake - %d", errno);
	i2c_fd = open("/dev/i2c-5", O_RDWR);
	LOGE_IF(i2c_fd < 0, "TScontrol: Cannot open i2c dev - %d", errno);

	return 0;
}

static struct hw_module_methods_t lights_module_methods = {
	.open =  open_lights,
};

const struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "lights Module",
	.author = "Google, Inc.",
	.methods = &lights_module_methods,
};
