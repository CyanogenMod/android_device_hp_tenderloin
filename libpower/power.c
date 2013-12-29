/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012 The CyanogenMod Project
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "Tenderloin PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include <sys/socket.h>
#include <sys/un.h>

#define SCALING_GOVERNOR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define BOOSTPULSE_ONDEMAND "/sys/devices/system/cpu/cpufreq/ondemand/boostpulse"
#define BOOSTPULSE_INTERACTIVE "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define NOTIFY_ON_MIGRATE "/dev/cpuctl/apps/cpu.notify_on_migrate"

#define TS_SOCKET_LOCATION "/dev/socket/tsdriver"
#define TS_SOCKET_DEBUG 0

static int ts_state;
static char governor[20];

struct tenderloin_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
};

static char governor[20];

static int sysfs_read(char *path, char *s, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);

        return -1;
    }

    if ((count = read(fd, s, num_bytes - 1)) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);

        ret = -1;
    } else {
        s[count] = '\0';
    }

    close(fd);

    return ret;
}

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static int get_scaling_governor() {
    if (sysfs_read(SCALING_GOVERNOR_PATH, governor,
                sizeof(governor)) == -1) {
        return -1;
    } else {
        // Strip newline at the end.
        int len = strlen(governor);

        len--;

        while (len >= 0 && (governor[len] == '\n' || governor[len] == '\r'))
            governor[len--] = '\0';
    }

    return 0;
}

/* connects to the touchscreen socket */
static void send_ts_socket(char *send_data) {
    struct sockaddr_un unaddr;
    int ts_fd, len;

    ts_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (ts_fd >= 0) {
        unaddr.sun_family = AF_UNIX;
        strcpy(unaddr.sun_path, TS_SOCKET_LOCATION);
        len = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
        if (connect(ts_fd, (struct sockaddr *)&unaddr, len) >= 0) {
#if TS_SOCKET_DEBUG
            ALOGD("Send ts socket %i byte(s): '%s'\n", sizeof(*send_data), send_data);
#endif
            send(ts_fd, send_data, sizeof(*send_data), 0);
        }
        close(ts_fd);
    }
}

static void tenderloin_power_set_interactive(struct power_module *module, int on)
{
    if (strncmp(governor, "ondemand", 8) == 0)
        sysfs_write(NOTIFY_ON_MIGRATE, on ? "1" : "0");

    /* tell touchscreen to turn on or off */
    if (on && ts_state == 0) {
        ALOGI("Enabling touch screen");
        ts_state = 1;
        send_ts_socket("O");
    } else if (!on && ts_state == 1) {
        ALOGI("Disabling touch screen");
        ts_state = 0;
        send_ts_socket("C");
    }
}

static void configure_governor()
{
    tenderloin_power_set_interactive(NULL, 1);

    if (strncmp(governor, "ondemand", 8) == 0) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/up_threshold", "90");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/io_is_busy", "1");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor", "2");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/down_differential", "10");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/up_threshold_multi_core", "70");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/down_differential_multi_core", "3");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/optimal_freq", "918000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/sync_freq", "1026000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/up_threshold_any_cpu_load", "80");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/input_boost", "1134000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/ondemand/sampling_rate", "50000");

    } else if (strncmp(governor, "interactive", 11) == 0) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time", "90000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/io_is_busy", "1");
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq", "1134000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay", "30000");
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate", "30000");
    }
}

static int boostpulse_open(struct tenderloin_power_module *tenderloin)
{
    char buf[80];

    pthread_mutex_lock(&tenderloin->lock);

    if (tenderloin->boostpulse_fd < 0) {
        if (get_scaling_governor() < 0) {
            ALOGE("Can't read scaling governor.");
            tenderloin->boostpulse_warned = 1;
        } else {
            if (strncmp(governor, "ondemand", 8) == 0)
                tenderloin->boostpulse_fd = open(BOOSTPULSE_ONDEMAND, O_WRONLY);
            else if (strncmp(governor, "interactive", 11) == 0)
                tenderloin->boostpulse_fd = open(BOOSTPULSE_INTERACTIVE, O_WRONLY);

            if (tenderloin->boostpulse_fd < 0 && !tenderloin->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGV("Error opening boostpulse: %s\n", buf);
                tenderloin->boostpulse_warned = 1;
            } else if (tenderloin->boostpulse_fd > 0) {
                configure_governor();
                ALOGD("Opened %s boostpulse interface", governor);
            }
        }
    }

    pthread_mutex_unlock(&tenderloin->lock);
    return tenderloin->boostpulse_fd;
}

static void tenderloin_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    struct tenderloin_power_module *tenderloin = (struct tenderloin_power_module *) module;
    char buf[80];
    int len;
    int duration = 1;

    switch (hint) {
    case POWER_HINT_INTERACTION:
    case POWER_HINT_CPU_BOOST:
        if (boostpulse_open(tenderloin) >= 0) {
            if (data != NULL)
                duration = (int) data;

            snprintf(buf, sizeof(buf), "%d", duration);
            len = write(tenderloin->boostpulse_fd, buf, strlen(buf));

            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to boostpulse: %s\n", buf);

                pthread_mutex_lock(&tenderloin->lock);
                close(tenderloin->boostpulse_fd);
                tenderloin->boostpulse_fd = -1;
                tenderloin->boostpulse_warned = 0;
                pthread_mutex_unlock(&tenderloin->lock);
            }
        }
        break;

    case POWER_HINT_VSYNC:
        break;

    default:
        break;
    }
}

static void tenderloin_power_init(struct power_module *module)
{
    get_scaling_governor();
    configure_governor();
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};


struct tenderloin_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "TouchPad Power HAL",
            author: "The CyanogenMod Project",
            methods: &power_module_methods,
        },
       init: tenderloin_power_init,
       setInteractive: tenderloin_power_set_interactive,
       powerHint: tenderloin_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: -1,
    boostpulse_warned: 0,
};
