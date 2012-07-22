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

#include <cutils/log.h>

#include "lsm303dlh_acc.h"

#define POLL_RATE 50000000 //50ms

Lsm303dlhGSensor::Lsm303dlhGSensor()
: SensorBase(LSM303DLH_ACC_DEVICE_NAME, "lsm303dlh_acc_sysfs"),
      mEnabled(0),
      mInputReader(32)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mEnabled = isEnabled();
}

Lsm303dlhGSensor::~Lsm303dlhGSensor() {
}

int Lsm303dlhGSensor::enable(int32_t handle, int en)
{
    int err = 0;

    int newState = en ? 1 : 0;

    // don't set enable state if it's already valid
    if(mEnabled == newState) {
        return err;
    }

    // ok we need to set our enabled state
    int fd = open(LSM303DLH_ACC_ENABLE_FILE, O_WRONLY);
    if(fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%d\n", newState);
        err = write(fd, buffer, bytes);
        err = err < 0 ? -errno : 0;
    } else {
        err = -errno;
    }

    ALOGE_IF(err < 0, "Error setting enable of LSM303DLH accelerometer (%s)"
            , strerror(-err));

    if (!err) {
        mEnabled = newState;
        //setDelay(0, POLL_RATE);
    }

    return err;
}

int Lsm303dlhGSensor::setDelay(int32_t handle, int64_t ns)
{
    int err = 0;

    if (mEnabled) {
        if (ns < 0)
            return -EINVAL;

        // ok we need to set our enabled state
        int fd = open(LSM303DLH_ACC_DELAY_FILE, O_WRONLY);
        if(fd >= 0) {
            char buffer[20];
            int bytes = sprintf(buffer, "%d\n", 100/*ns / (100 * 100)*/);
            err = write(fd, buffer, bytes);
            err = err < 0 ? -errno : 0;
        } else {
            err = -errno;
        }

		close(fd);

        ALOGE_IF(err < 0,
                "Error setting delay of LSM303DLH accelerometer (%s)",
                strerror(-err)); } 
    return err;
}

int Lsm303dlhGSensor::readEvents(sensors_event_t* data, int count)
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
            ALOGE("LSM303DLH_ACC: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

void Lsm303dlhGSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = value * CONVERT_A_X;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = value * CONVERT_A_Z;
            break;
    }
}

int Lsm303dlhGSensor::isEnabled()
{
    int fd = open(LSM303DLH_ACC_ENABLE_FILE, O_RDONLY);
    if (fd >= 0) {
        char buffer[20];
        int amt = read(fd, buffer, 20);
        close(fd);
        if(amt > 0) {
            return (buffer[0] == '1');
        } else {
            ALOGE("LSM303DLH_ACC: isEnable() failed to read (%s)",
                    strerror(errno));
            return 0;
        }
    } else {
        ALOGE("LSM303DLH_ACC: isEnabled failed to open %s",
                LSM303DLH_ACC_ENABLE_FILE);
        return 0;
    }
}
