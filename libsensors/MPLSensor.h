/*
 * Copyright (C) 2011 Invensense, Inc.
 * Copyright (C) 2012 Tomasz Rostanski
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
/*************Removed the gesture related info for Google check in : Meenakshi Ramamoorthi: May 31st *********/

#ifndef ANDROID_MPL_SENSOR_H
#define ANDROID_MPL_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <poll.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include "nusensors.h"
#include "SensorBase.h"

/*****************************************************************************/
/** MPLSensor implementation which fits into the HAL example for crespo provided
 * * by Google.
 * * WARNING: there may only be one instance of MPLSensor, ever.
 */

class MPLSensor: public SensorBase
{
    typedef void (MPLSensor::*hfunc_t)(sensors_event_t*, uint32_t*, int);

public:
    MPLSensor();
    virtual ~MPLSensor();

    enum
    {
        Gyro=0,
        Temp,
        numSensors
    };

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t *data, int count);
    virtual int getFd() const;
    virtual int getPollTime();
    virtual bool hasPendingEvents() const;
    void cbOnMotion(uint16_t);
    void cbProcData();

protected:

    void clearIrqData(bool* irq_set);
    void setPowerStates(int enabledsensor);
    void initMPL();
    void setupFIFO();
    void setupCallbacks();
    void gyroHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void tempHandler(sensors_event_t *data, uint32_t *pendmask, int index);

    int mMpuAccuracy; //global storage for the current accuracy status
    int mNewData; //flag indicating that the MPL calculated new output values
    int mDmpStarted;
    long mMasterSensorMask;
    long mLocalSensorMask;
    int mPollTime;
    int mCurFifoRate; //current fifo rate
    bool mHaveGoodMpuCal; //flag indicating that the cal file can be written
    struct pollfd mPollFds[1];
    int mSampleCount;
    pthread_mutex_t mMplMutex;
    int64_t now_ns();
    int64_t select_ns(unsigned long long time_set[]);

    enum FILEHANDLES
    {
        MPUIRQ_FD
    };

private:

    int update_delay();

    uint32_t mEnabled;
    uint32_t mPendingMask;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
    hfunc_t mHandlers[numSensors];
    bool mForceSleep;
    long int mOldEnabledMask;
    android::KeyedVector<int, int> mIrqFds;
};

void setCallbackObject(MPLSensor*);

/*****************************************************************************/

#endif  // ANDROID_MPL_SENSOR_H
