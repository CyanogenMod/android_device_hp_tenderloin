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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <linux/input.h>

#include <cutils/log.h>

#include "lsm303dlh_mag.h"

#define POLL_RATE 200000000 //200ms

Lsm303dlhMagSensor::Lsm303dlhMagSensor()
: SensorBase(LSM303DLH_MAG_DEVICE_NAME, "lsm303dlh_mag_sysfs"),
      mEnabled(0),
      mInputReader(32)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_M;
    mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    mPendingEvent.magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mEnabled = isEnabled();
}

Lsm303dlhMagSensor::~Lsm303dlhMagSensor() {
}

int Lsm303dlhMagSensor::enable(int32_t handle, int en)
{
    int err = 0;

    unsigned int newState = en ? 1 : 0;

    // don't set enable state if it's already valid
    if(mEnabled == newState) {
        return err;
    }

    // ok we need to set our enabled state
    int fd = open(LSM303DLH_MAG_ENABLE_FILE, O_WRONLY);
    if(fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%u\n", newState);
        err = write(fd, buffer, bytes);
        err = err < 0 ? -errno : 0;
        close(fd);
    } else {
        err = -errno;
    }

    ALOGE_IF(err < 0, "Error setting enable of LSM303DLH magnetometer (%s)"
            , strerror(-err));

    if (!err) {
        mEnabled = newState;
        setDelay(0, POLL_RATE); 
    }

    return err;
}

int Lsm303dlhMagSensor::setDelay(int32_t handle, int64_t ns)
{
    int err = 0;

    if (mEnabled) {
        if (ns < 0)
            return -EINVAL;

        unsigned long delay = ns / 1000000; //nano to mili

        // ok we need to set our enabled state
        int fd = open(LSM303DLH_MAG_DELAY_FILE, O_WRONLY);
        if(fd >= 0) {
            char buffer[20];
            int bytes = sprintf(buffer, "%lu\n", delay);
            err = write(fd, buffer, bytes);
            err = err < 0 ? -errno : 0;
            close(fd);
        } else {
            err = -errno;
        }

        ALOGE_IF(err < 0,
                "Error setting delay of LSM303DLH magnetometer (%s)",
                strerror(-err)); } 
    return err;
}

int Lsm303dlhMagSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            *data++ = mPendingEvent;
            count--;
            numEventReceived++;
        } else {
            ALOGE("LSM303DLH_MAG: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

void Lsm303dlhMagSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_MAGV_X:
            mPendingEvent.magnetic.x = value * CONVERT_M_X;
            break;
        case EVENT_TYPE_MAGV_Y:
            mPendingEvent.magnetic.y = value * CONVERT_M_Y;
            break;
        case EVENT_TYPE_MAGV_Z:
            mPendingEvent.magnetic.z = value * CONVERT_M_Z;
            break;
    }
}

int Lsm303dlhMagSensor::isEnabled()
{
    int fd = open(LSM303DLH_MAG_ENABLE_FILE, O_RDONLY);
    if (fd >= 0) {
        char buffer[20];
        int amt = read(fd, buffer, 20);
        close(fd);
        if(amt > 0) {
            return (buffer[0] == '1');
        } else {
            ALOGE("LSM303DLH_MAG: isEnable() failed to read (%s)",
                    strerror(errno));
            return 0;
        }
    } else {
        ALOGE("LSM303DLH_MAG: isEnabled failed to open %s",
                LSM303DLH_MAG_ENABLE_FILE);
        return 0;
    }
}
