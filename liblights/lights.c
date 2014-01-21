/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 <kang@insecure.ws>
 * Copyright (C) 2012 James Sullins <jcsullins@gmail.com>
 * Copyright (C) 2013 Micha LaQua <micha.laqua@gmail.com>
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

//#define LOG_NDEBUG 0
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

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int g_btnled_on = 0;
static int g_notled_on = 0;
static int g_batled_on = 0;

char const *const LCD_FILE = "/sys/class/leds/lcd-backlight/brightness";
char const *const LED_FILE = "/dev/lm8502";

char const *const LEFTNAVI_FILE = "/sys/class/leds/core_navi_left/brightness";
char const *const RIGHTNAVI_FILE = "/sys/class/leds/core_navi_right/brightness";

/* copied from kernel include/linux/i2c_lm8502_led.h */
#define LM8502_DOWNLOAD_MICROCODE       1
#define LM8502_START_ENGINE             9
#define LM8502_STOP_ENGINE              3
#define LM8502_WAIT_FOR_ENGINE_STOPPED  8

/* LED engine programs */
static const uint16_t notif_led_program_pulse_quick[] = {
    0x9c0f, 0x9c8f, 0xe004, 0x4000, 0x047f, 0x4c00, 0x047f, 0x4c00,
    0x047f, 0x4c00, 0x057f, 0x4c00, 0xa30a, 0x0000, 0x0000, 0x0007,
    0x9c1f, 0x9c9f, 0xe080, 0x03ff, 0xc800
};
static const uint16_t notif_led_program_pulse_quickshort[] = {
    0x9c0f, 0x9c8f, 0xe004, 0x4000, 0x047f, 0x4c00, 0x057f, 0x4c00,
    0x057f, 0x4c00, 0x057f, 0x4c00, 0xa30a, 0x0000, 0x0000, 0x0007,
    0x9c1f, 0x9c9f, 0xe080, 0x03ff, 0xc800
};
static const uint16_t notif_led_program_pulse_long[] = {
    0x9c0f, 0x9c8f, 0xe004, 0x4000, 0x047f, 0x4c00, 0x047f, 0x4c00,
    0x057f, 0x4c00, 0x057f, 0x7c00, 0xa30a, 0x0000, 0x0000, 0x0007,
    0x9c1f, 0x9c9f, 0xe080, 0x03ff, 0xc800
};
static const uint16_t notif_led_program_pulse_longshort[] = {
    0x9c0f, 0x9c8f, 0xe004, 0x4000, 0x047f, 0x4c00, 0x057f, 0x4c00,
    0x057f, 0x4c00, 0x057f, 0x6c00, 0xa30a, 0x0000, 0x0000, 0x0007,
    0x9c1f, 0x9c9f, 0xe080, 0x03ff, 0xc800
};
static const uint16_t notif_led_program_pulse_double[] = {
    0x9c0f, 0x9c8f, 0xe004, 0x4000, 0x047f, 0x4c00, 0x057f, 0x4c00,
    0x047f, 0x4c00, 0x057f, 0x7c00, 0xa30a, 0x0000, 0x0000, 0x0007,
    0x9c1f, 0x9c9f, 0xe080, 0x03ff, 0xc800
};
static const uint16_t notif_led_program_reset[] = {
    0x9c0f, 0x9c8f, 0x03ff, 0xc000
};

int
is_discharging()
{
    int ret;

    FILE* fp = fopen("/sys/class/power_supply/battery/status", "r");
    if (!fp) {
        ALOGV("is_charging failed to open /sys/class/power_supply/battery/status - assuming discharging\n");
        return 1;
    } else {
		if ((char)(fgetc(fp)) == 'D')
            ret = 1;
        else
            ret = 0;

        fclose(fp);
        return ret;
    }
}

static int set_navled(int on)
{
	if (on) {
		write_int(LEFTNAVI_FILE, 25);
		write_int(RIGHTNAVI_FILE, 25);
	} else {
		write_int(LEFTNAVI_FILE, 0);
		write_int(RIGHTNAVI_FILE, 0);
	}
		
}

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned;

	already_warned = 0;

	ALOGV("write_int: path %s, value %d", path, value);
	fd = open(path, O_RDWR);

	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
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

static int
is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static void init_notification_led(void)
{
	uint16_t microcode[96];
	int fd;

	memset(microcode, 0, sizeof(microcode));

	fd = open(LED_FILE, O_RDWR);
	if (fd < 0) {
		ALOGE("Cannot open notification LED device - %d", errno);
		return;
	}

	/* download microcode */
	if (ioctl(fd, LM8502_DOWNLOAD_MICROCODE, microcode) < 0) {
		ALOGE("Cannot download LED microcode - %d", errno);
		return;
	}
	if (ioctl(fd, LM8502_STOP_ENGINE, 1) < 0) {
		ALOGE("Cannot stop LED engine 1 - %d", errno);
	}
	if (ioctl(fd, LM8502_STOP_ENGINE, 2) < 0) {
		ALOGE("Cannot stop LED engine 2 - %d", errno);
	}

	close(fd);
}

static int
program_notification_led(int state)
{
	uint16_t microcode[96];
	int fd, ret;

	ALOGV("%sabling notification light", state ? "En" : "Dis");

	memset(microcode, 0, sizeof(microcode));

	if (state == 1) {
		memcpy(microcode, notif_led_program_pulse_quick, sizeof(notif_led_program_pulse_quick));
	} else if (state == 2) {
		memcpy(microcode, notif_led_program_pulse_quickshort, sizeof(notif_led_program_pulse_quickshort));
	} else if (state == 3) {
		memcpy(microcode, notif_led_program_pulse_long, sizeof(notif_led_program_pulse_long));
	} else if (state == 4) {
		memcpy(microcode, notif_led_program_pulse_longshort, sizeof(notif_led_program_pulse_longshort));
	} else if (state == 5) {
		memcpy(microcode, notif_led_program_pulse_double, sizeof(notif_led_program_pulse_double));
	} else {
		memcpy(microcode, notif_led_program_reset, sizeof(notif_led_program_reset));
	}

	fd = open(LED_FILE, O_RDWR);
	if (fd < 0) {
		ALOGE("Opening %s failed - %d", LED_FILE, errno);
		ret = -errno;
	} else {
		ret = 0;

		/* download microcode */
		if (ioctl(fd, LM8502_DOWNLOAD_MICROCODE, microcode) < 0) {
			ALOGE("Copying notification microcode failed - %d", errno);
			ret = -errno;
		}
		if (ret == 0 && state) {
			if (ioctl(fd, LM8502_START_ENGINE, 2) < 0) {
				ALOGE("Starting notification LED engine 2 failed - %d", errno);
				ret = -errno;
			}
		}
		if (ret == 0) {
			if (ioctl(fd, LM8502_START_ENGINE, 1) < 0) {
				ALOGE("Starting notification LED engine 1 failed - %d", errno);
				ret = -errno;
			}
		}
		if (ret == 0 && !state) {
		    ALOGV("stop engine 1");
			if (ioctl(fd, LM8502_STOP_ENGINE, 1) < 0) {
				ALOGE("Stopping notification LED engine 1 failed - %d", errno);
				ret = -errno;
			}
			if (ioctl(fd, LM8502_STOP_ENGINE, 2) < 0) {
				ALOGE("Stopping notification LED engine 2 failed - %d", errno);
				ret = -errno;
			}
		}
		if (ret == 0 && !state) {
			ALOGV("Waiting for notification LED engine to stop after reset");
			int state;
			/* make sure the reset is complete */
			if (ioctl(fd, LM8502_WAIT_FOR_ENGINE_STOPPED, &state) < 0) {
				ALOGW("Waiting for notification LED reset failed - %d", errno);
				ret = -errno;
			}
			ALOGV("Notification LED reset finished with stop state %d", state);
		}

		close(fd);
	}

	return ret;

}

static int
set_light_buttons(struct light_device_t* dev,
        struct light_state_t const* state)
{
	ALOGV("set_light_buttons called with state = %d", g_btnled_on);

    pthread_mutex_lock(&g_lock);
    g_btnled_on = is_lit(state);
    if (g_btnled_on)
        set_navled(1);
    else if (!g_btnled_on && !g_notled_on && !g_batled_on)
        set_navled(0);
    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_notifications(struct light_device_t* dev,
			struct light_state_t const* state)
{
	ALOGV("set_light_notifications called with state = %d", g_notled_on);
	ALOGV("flashOFF %i, flashON: %i", state->flashOffMS, state->flashOnMS);

    pthread_mutex_lock(&g_lock);
    g_notled_on = is_lit(state);
    if (g_notled_on) {
		if (state->flashOnMS == 1) {
			set_navled(1);
		} else if (state->flashOffMS < 2500) {
			if (state->flashOnMS >= 1000) {
				program_notification_led(1);
			} else {
				program_notification_led(2);
			}
		} else if (state->flashOffMS >= 2500) {
			if (state->flashOnMS >= 1000) {
				program_notification_led(3);
			} else {
				program_notification_led(4);
			}
		}
	} else if (!g_notled_on) {
		program_notification_led(0);
		if (g_btnled_on || g_batled_on)
			set_navled(1);
	}
    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_battery (struct light_device_t* dev,
			struct light_state_t const* state)
{
	unsigned int colorRGB = state->color & 0xFFFFFF;
	int red = (colorRGB >> 16)&0xFF;

    ALOGV("set_light_battery called with state = %d", g_batled_on);

    pthread_mutex_lock(&g_lock);
	g_batled_on = red&&!is_discharging();

    if (g_batled_on && !g_notled_on)
        set_navled(1);
    else if (!g_batled_on && !g_notled_on && !g_btnled_on)
        set_navled(0);
    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_backlight(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);

	pthread_mutex_lock(&g_lock);
	err = write_int(LCD_FILE, brightness);
	pthread_mutex_unlock(&g_lock);
	return err;
}

static int close_lights(struct light_device_t *dev)
{
	ALOGV("close_light is called");
	if (dev)
		free(dev);

	return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
						struct hw_device_t **device)
{
	int (*set_light)(struct light_device_t *dev,
		struct light_state_t const *state);

	ALOGV("open_lights: open with %s", name);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;
	else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
		set_light = set_light_buttons;
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
		set_light = set_light_notifications;
	else if (0 == strcmp(LIGHT_ID_BATTERY, name))
		set_light = set_light_battery;
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

	init_notification_led();

	return 0;
}

static struct hw_module_methods_t lights_module_methods = {
	.open =  open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "lights Module",
	.author = "Google, Inc.",
	.methods = &lights_module_methods,
};

