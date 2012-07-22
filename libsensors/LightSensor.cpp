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
#include <linux/delay.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "LightSensor.h"

/*****************************************************************************/

/* The Crespo ADC sends 4 somewhat bogus events after enabling the sensor.
   This becomes a problem if the phone is turned off in bright light
   and turned back on in the dark.
   To avoid this we ignore the first 4 events received after enabling the sensor.
 */
#define FIRST_GOOD_EVENT    1

LightSensor::LightSensor()
    : SensorBase(NULL, "isl29023 light sensor"),
      mEnabled(0),
      mEventsSinceEnable(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, "event4");
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
        enable(0, 1);
    }
}

LightSensor::~LightSensor() {
    if (mEnabled) {
        enable(0, 0);
    }
}

int LightSensor::setDelay(int32_t handle, int64_t ns)
{
    //Sysfs delay not implemented yet

    /*
    int fd;
    strcpy(&input_sysfs_path[input_sysfs_path_len], "poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
    */
    return 0;
}

int LightSensor::enable(int32_t handle, int en)
{
    int flags = en ? 1 : 0;
    mEventsSinceEnable = 0;
    mPreviousLight = -1;
    if (flags != mEnabled) {
        int fd;
        int fd_mode; 
        fd = open(ISL29023_ENABLE_FILE, O_RDWR);
        fd_mode = open(ISL29023_MODE_FILE, O_RDWR);
        if (fd >= 0 && fd_mode >= 0) {
            char buf_enable[2];
            char buf_mode[2];
            int err;
            buf_enable[1] = 0;
            buf_mode[1] = 0;
            if (flags) {
                buf_enable[0] = '1';
                buf_mode[0] = '5';
            } else {
                buf_enable[0] = '0';
            }
            if(!(err = write(fd, buf_enable, sizeof(buf_enable)))) {
                ALOGE("LightSensor: unable to write to %s", 
                        ISL29023_ENABLE_FILE);
            }
            if(!(err = write(fd_mode, buf_mode, sizeof(buf_mode)))) {
                ALOGE("LightSensor: unable to write to %s", 
                        ISL29023_MODE_FILE);
            }
            
            close(fd);
            close(fd_mode);
            mEnabled = flags;
            return 0;
        }
        return -1;
    }
    return 0;
}

bool LightSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == EVENT_TYPE_LIGHT) {
                mPendingEvent.light = event->value;
                if (mEventsSinceEnable < FIRST_GOOD_EVENT)
                    mEventsSinceEnable++;
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled && (mPendingEvent.light != mPreviousLight) &&
                    mEventsSinceEnable >= FIRST_GOOD_EVENT) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
                mPreviousLight = mPendingEvent.light;
            }
        } else {
            ALOGE("LightSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}
