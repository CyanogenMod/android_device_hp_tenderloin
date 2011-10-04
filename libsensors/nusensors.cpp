/*
 * Copyright (C) 2008 The Android Open Source Project
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
#define LOG_NDEBUG 0
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <cutils/atomic.h>
#include <cutils/log.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "nusensors.h"
#include "lsm303dlh_acc.h"
#include "lsm303dlh_mag.h"
#include "LightSensor.h"
/*****************************************************************************/

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        lsm303dlh_acc           = 0,
        lsm303dlh_mag           = 1,
        isl29023_als            = 2,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
            	return lsm303dlh_acc;
            case ID_M:
                return lsm303dlh_mag;
            case ID_L:
                return isl29023_als;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[lsm303dlh_acc] = new Lsm303dlhGSensor();
    mPollFds[lsm303dlh_acc].fd = mSensors[lsm303dlh_acc]->getFd();
    mPollFds[lsm303dlh_acc].events = POLLIN;
    mPollFds[lsm303dlh_acc].revents = 0;

    mSensors[lsm303dlh_mag] = new Lsm303dlhMagSensor();
    mPollFds[lsm303dlh_mag].fd = mSensors[lsm303dlh_mag]->getFd();
    mPollFds[lsm303dlh_mag].events = POLLIN;
    mPollFds[lsm303dlh_mag].revents = 0;

    mSensors[isl29023_als] = new LightSensor();
    mPollFds[isl29023_als].fd = mSensors[isl29023_als]->getFd();
    mPollFds[isl29023_als].events = POLLIN;
    mPollFds[isl29023_als].revents = 0;



    int wakeFds[2];
    int result = pipe(wakeFds);
    LOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
LOGD("sensor activation called: handle=%d, enabled=%d********************************", handle, enabled);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        LOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }

    /* TS power magic hack, ts power tracks that of accelerometer */
    if (handle == ID_A)
        touchscreen_power(enabled);

    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                LOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                LOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                LOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }

    touchscreen_power(0);

    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/** TS power stuff */
static int vdd_fd, xres_fd, wake_fd, i2c_fd;

void touchscreen_power(int enable)
{
    struct i2c_rdwr_ioctl_data i2c_ioctl_data;
    struct i2c_msg i2c_msg;
    __u8 i2c_buf[16];

    if (enable) {
        lseek(vdd_fd, 0, SEEK_SET);
        write(vdd_fd, "1", 1);

        lseek(wake_fd, 0, SEEK_SET);
        write(wake_fd, "1", 1);

        lseek(xres_fd, 0, SEEK_SET);
        write(xres_fd, "1", 1);

        lseek(xres_fd, 0, SEEK_SET);
        write(xres_fd, "0", 1);

        usleep(50000);

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
    } else {
        lseek(vdd_fd, 0, SEEK_SET);
        write(vdd_fd, "0", 1);
        /* XXX, should be correllated with LIFTOFF_TIMEOUT in ts driver */
        usleep(80000);
    }
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
    int status = -EINVAL;

    sensors_poll_context_t *dev = new sensors_poll_context_t();
    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    /* TS file descriptors. Ignore errors. */
    vdd_fd = open("/sys/devices/platform/cy8ctma395/vdd", O_WRONLY);
    LOGE_IF(vdd_fd < 0, "TScontrol: Cannot open vdd - %d", errno);
    xres_fd = open("/sys/devices/platform/cy8ctma395/xres", O_WRONLY);
    LOGE_IF(xres_fd < 0, "TScontrol: Cannot open xres - %d", errno);
    wake_fd = open("/sys/user_hw/pins/ctp/wake/level", O_WRONLY);
    LOGE_IF(wake_fd < 0, "TScontrol: Cannot open wake - %d", errno);
    i2c_fd = open("/dev/i2c-5", O_RDWR);
    LOGE_IF(i2c_fd < 0, "TScontrol: Cannot open i2c dev - %d", errno);
    return status;
}
