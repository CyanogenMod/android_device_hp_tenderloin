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

//#define ALOG_NDEBUG 0
//see also the EXTRA_VERBOSE define, below

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <pthread.h>

#include <cutils/log.h>
#include <utils/KeyedVector.h>

#include "MPLSensor.h"

#include "mlFIFO.h"
#include "ml_stored_data.h"
#include "mldl_cfg.h"
#include "mldl.h"

#include "mpu.h"
#include "kernel/mpuirq.h"

#define EXTRA_VERBOSE (0)
//#define FUNC_ALOG ALOGV("%s", __PRETTY_FUNCTION__)
#define FUNC_ALOG
#define VFUNC_ALOG ALOGV_IF(EXTRA_VERBOSE, "%s", __PRETTY_FUNCTION__)

#define CALL_MEMBER_FN(pobject,ptrToMember)  ((pobject)->*(ptrToMember))

static unsigned long long irq_timestamp = 0;
/* ***************************************************************************
 * MPL interface misc.
 */
//static pointer to the object that will handle callbacks
static MPLSensor* gMPLSensor = NULL;

/* we need to pass some callbacks to the MPL.  The mpl is a C library, so
 * wrappers for the C++ callback implementations are required.
 */
extern "C" {
//callback wrappers go here
void mot_cb_wrapper(uint16_t val)
{
    if (gMPLSensor) {
        gMPLSensor->cbOnMotion(val);
    }
}

void procData_cb_wrapper()
{
    if(gMPLSensor) {
        gMPLSensor->cbProcData();
    }
}

} //end of extern C

void setCallbackObject(MPLSensor* gbpt)
{
    gMPLSensor = gbpt;
}


/*****************************************************************************
 * sensor class implementation
 */


MPLSensor::MPLSensor() :
    SensorBase(NULL, NULL),
            mMpuAccuracy(0), mNewData(0),
            mDmpStarted(false),
            mMasterSensorMask(INV_THREE_AXIS_GYRO),
            mLocalSensorMask(INV_THREE_AXIS_GYRO), mPollTime(-1),
            mCurFifoRate(-1), mHaveGoodMpuCal(false),
            mSampleCount(0), mEnabled(0), mPendingMask(0)
{
    FUNC_ALOG;
    inv_error_t rv;
    int mpu_int_fd;
    unsigned i;
    char *port = NULL;

    ALOGV_IF(EXTRA_VERBOSE, "MPLSensor constructor: numSensors = %d", numSensors);

    pthread_mutex_init(&mMplMutex, NULL);

    mForceSleep = false;

    for (i = 0; i < ARRAY_SIZE(mPollFds); i++) {
        mPollFds[i].fd = -1;
        mPollFds[i].events = 0;
    }

    pthread_mutex_lock(&mMplMutex);

    mpu_int_fd = open("/dev/mpuirq", O_RDWR);
    if (mpu_int_fd == -1) {
        ALOGE("could not open the mpu irq device node");
    } else {
        fcntl(mpu_int_fd, F_SETFL, O_NONBLOCK);
        //ioctl(mpu_int_fd, MPUIRQ_SET_TIMEOUT, 0);
        mIrqFds.add(MPUIRQ_FD, mpu_int_fd);
        mPollFds[MPUIRQ_FD].fd = mpu_int_fd;
        mPollFds[MPUIRQ_FD].events = POLLIN;
    }
    data_fd = mpu_int_fd;

    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[Gyro].version = sizeof(sensors_event_t);
    mPendingEvents[Gyro].sensor = ID_GY;
    mPendingEvents[Gyro].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[Gyro].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Temp].version = sizeof(sensors_event_t);
    mPendingEvents[Temp].sensor = ID_T;
    mPendingEvents[Temp].type = SENSOR_TYPE_TEMPERATURE;
    mPendingEvents[Temp].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mHandlers[Gyro] = &MPLSensor::gyroHandler;
    mHandlers[Temp] = &MPLSensor::tempHandler;

    for (int i = 0; i < numSensors; i++)
        mDelays[i] = 30000000LLU; // 30 ms by default

    if (inv_serial_start(port) != INV_SUCCESS) {
        ALOGE("Fatal Error : could not open MPL serial interface");
    }

    //initialize library parameters
    initMPL();

    //setup the FIFO contents
    setupFIFO();

    //setup callback object
    setCallbackObject(this);

    pthread_mutex_unlock(&mMplMutex);
}

MPLSensor::~MPLSensor()
{
    FUNC_ALOG;
    pthread_mutex_lock(&mMplMutex);
    if (inv_dmp_stop() != INV_SUCCESS) {
        ALOGW("Error: could not stop the DMP correctly.\n");
    }

    if (inv_dmp_close() != INV_SUCCESS) {
        ALOGW("Error: could not close the DMP");
    }

    if (inv_serial_stop() != INV_SUCCESS) {
        ALOGW("Error : could not close the serial port");
    }
    pthread_mutex_unlock(&mMplMutex);
    pthread_mutex_destroy(&mMplMutex);
}

/* clear any data from our various filehandles */
void MPLSensor::clearIrqData(bool* irq_set)
{
    unsigned int i;
    int nread;
    struct mpuirq_data irqdata;

    poll(mPollFds, ARRAY_SIZE(mPollFds), 0); //check which ones need to be cleared

    for (i = 0; i < ARRAY_SIZE(mPollFds); i++) {
        int cur_fd = mPollFds[i].fd;
        int j = 0;
        if (mPollFds[i].revents & POLLIN) {
            nread = read(cur_fd, &irqdata, sizeof(irqdata));
            if (nread > 0) {
                irq_set[i] = true;

                irq_timestamp = irqdata.irqtime;
                ALOGV_IF(EXTRA_VERBOSE, "irq: %d %d (%d)", i, irqdata.interruptcount, j++);
            }
        }
        mPollFds[i].revents = 0;
    }
}

/* set the power states of the various sensors based on the bits set in the
 * enabled_sensors parameter.
 * this function modifies globalish state variables.  It must be called with the mMplMutex held. */
void MPLSensor::setPowerStates(int enabled_sensors)
{
    FUNC_ALOG;
    bool irq_set[5] = { false, false, false, false, false };

    ALOGV(" setPowerStates: %d dmp_started: %d", enabled_sensors, mDmpStarted);

    if (enabled_sensors) {
        ALOGI(" Enabling gyroscope");
        mLocalSensorMask = INV_THREE_AXIS_GYRO;
    } else {
        ALOGI(" Disabling gyroscope");
        mLocalSensorMask = 0;
    }

    //record the new sensor state
    inv_error_t rv;

    unsigned long sen_mask = mLocalSensorMask & mMasterSensorMask;

    bool changing_sensors = ((inv_get_dl_config()->requested_sensors
            != sen_mask) && (sen_mask != 0));
    bool restart = (!mDmpStarted) && (sen_mask != 0);

    if (changing_sensors || restart) {

        ALOGV_IF(EXTRA_VERBOSE, "cs:%d rs:%d ", changing_sensors, restart);

        if (mDmpStarted) {
            inv_dmp_stop();
            clearIrqData(irq_set);
            mDmpStarted = false;
        }

        if (sen_mask != inv_get_dl_config()->requested_sensors) {
            ALOGV("setPowerStates: %lx", sen_mask);
            rv = inv_set_mpu_sensors(sen_mask);
            ALOGE_IF(rv != INV_SUCCESS,
                    "error: unable to set MPL sensor power states (sens=%ld retcode = %d)",
                    sen_mask, rv);
        }

        if (!mDmpStarted) {
            if (mHaveGoodMpuCal) {
                rv = inv_store_calibration();
                ALOGE_IF(rv != INV_SUCCESS,
                        "error: unable to store MPL calibration file");
                mHaveGoodMpuCal = false;
            }
            ALOGV("Starting DMP");
            rv = inv_dmp_start();
            ALOGE_IF(rv != INV_SUCCESS, "unable to start dmp");
            mDmpStarted = true;
        }
    }

    //check if we should stop the DMP
    if (mDmpStarted && (sen_mask == 0)) {
        ALOGV("Stopping DMP");
        rv = inv_dmp_stop();
        ALOGE_IF(rv != INV_SUCCESS, "error: unable to stop DMP (retcode = %d)",
                rv);

        clearIrqData(irq_set);

        mDmpStarted = false;
        mPollTime = -1;
        mCurFifoRate = -1;
    }
}

/**
 * container function for all the calls we make once to set up the MPL.
 */
void MPLSensor::initMPL()
{
    FUNC_ALOG;
    inv_error_t result;
    unsigned short bias_update_mask = 0xFFFF;
    struct mldl_cfg *mldl_cfg;

    if (inv_dmp_open() != INV_SUCCESS) {
        ALOGE("Fatal Error : could not open DMP correctly.\n");
    }

    result = inv_set_mpu_sensors(INV_THREE_AXIS_GYRO);
    ALOGE_IF(result != INV_SUCCESS,
            "Fatal Error : could not set enabled sensors.");

    if (inv_load_calibration() != INV_SUCCESS) {
        ALOGE("could not open MPL calibration file");
    }

    mldl_cfg = inv_get_dl_config();

    if (inv_set_bias_update(bias_update_mask) != INV_SUCCESS) {
        ALOGE("Error : Bias update function could not be set.\n");
    }

    if (inv_set_motion_interrupt(1) != INV_SUCCESS) {
        ALOGE("Error : could not set motion interrupt");
    }

    if (inv_set_fifo_interrupt(1) != INV_SUCCESS) {
        ALOGE("Error : could not set fifo interrupt");
    }

    result = inv_set_fifo_rate(6);
    if (result != INV_SUCCESS) {
        ALOGE("Fatal error: inv_set_fifo_rate returned %d\n", result);
    }

    mMpuAccuracy = SENSOR_STATUS_ACCURACY_MEDIUM;
    setupCallbacks();
}

/** setup the fifo contents.
 */
void MPLSensor::setupFIFO()
{
    FUNC_ALOG;
    inv_error_t result = inv_send_gyro(INV_ALL, INV_32_BIT);
    if (result != INV_SUCCESS) {
        ALOGE("Fatal error: inv_send_gyro returned %d\n", result);
    }
}

/**
 *  set up the callbacks that we use in all cases (outside of gestures, etc)
 */
void MPLSensor::setupCallbacks()
{
    FUNC_ALOG;
    if (inv_set_motion_callback(mot_cb_wrapper) != INV_SUCCESS) {
        ALOGE("Error : Motion callback could not be set.\n");

    }

    if (inv_set_fifo_processed_callback(procData_cb_wrapper) != INV_SUCCESS) {
        ALOGE("Error : Processed data callback could not be set.");

    }
}

/**
 * handle the motion/no motion output from the MPL.
 */
void MPLSensor::cbOnMotion(uint16_t val)
{
    FUNC_ALOG;
    //after the first no motion, the gyro should be calibrated well
    if (val == 2) {
        mMpuAccuracy = SENSOR_STATUS_ACCURACY_HIGH;
        if ((inv_get_dl_config()->requested_sensors) & INV_THREE_AXIS_GYRO) {
            //if gyros are on and we got a no motion, set a flag
            // indicating that the cal file can be written.
            mHaveGoodMpuCal = true;
        }
    }

    return;
}


void MPLSensor::cbProcData()
{
    mNewData = 1;
    mSampleCount++;
    ALOGV_IF(EXTRA_VERBOSE, "new data (%d)", sampleCount);
}

//these handlers transform mpl data into one of the Android sensor types
//  scaling and coordinate transforms should be done in the handlers

void MPLSensor::gyroHandler(sensors_event_t* s, uint32_t* pending_mask,
                             int index)
{
    VFUNC_ALOG;
    inv_error_t res;
    res = inv_get_float_array(INV_GYROS, s->gyro.v);
    s->gyro.v[0] = s->gyro.v[0] * M_PI / 180.0;
    s->gyro.v[1] = s->gyro.v[1] * M_PI / 180.0;
    s->gyro.v[2] = s->gyro.v[2] * M_PI / 180.0;
    s->gyro.status = mMpuAccuracy;
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

void MPLSensor::tempHandler(sensors_event_t* s, uint32_t* pending_mask,
                             int index)
{
    VFUNC_ALOG;
    inv_error_t res = inv_get_temperature_float(&s->temperature);
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

int MPLSensor::enable(int32_t handle, int en)
{
    FUNC_ALOG;

    int what = -1;
    switch (handle) {
    case ID_GY:
        what = Gyro;
        break;
    case ID_T:
        what = Temp;
        break;
    }
    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState = en ? 1 : 0;
    int err = 0;
    ALOGV_IF((uint32_t(newState) << what) != (mEnabled & (1 << what)),
            "sensor state change");

    pthread_mutex_lock(&mMplMutex);
    if ((uint32_t(newState) << what) != (mEnabled & (1 << what))) {
        uint32_t sensor_type;
        short flags = newState;
        mEnabled &= ~(1 << what);
        mEnabled |= (uint32_t(flags) << what);
        ALOGV_IF(EXTRA_VERBOSE, "mEnabled = %x", mEnabled);
        setPowerStates(mEnabled);
        pthread_mutex_unlock(&mMplMutex);
        if (!newState)
            update_delay();
        return err;
    }
    pthread_mutex_unlock(&mMplMutex);
    return err;
}

int MPLSensor::setDelay(int32_t handle, int64_t ns)
{
    FUNC_ALOG;
    ALOGV_IF(EXTRA_VERBOSE,
            " setDelay handle: %d rate %d", handle, (int) (ns / 1000000LL));
    int what = -1;
    switch (handle) {
    case ID_GY:
        what = Gyro;
        break;
    case ID_T:
        what = Temp;
        break;
    }
    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;

    pthread_mutex_lock(&mMplMutex);
    mDelays[what] = ns;
    pthread_mutex_unlock(&mMplMutex);
    return update_delay();
}

int MPLSensor::update_delay()
{
    FUNC_ALOG;
    int rv = 0;
    bool irq_set[5];

    pthread_mutex_lock(&mMplMutex);

    if (mEnabled) {
        uint64_t wanted = -1LLU;
        for (int i = 0; i < numSensors; i++) {
            if (mEnabled & (1 << i)) {
                uint64_t ns = mDelays[i];
                wanted = wanted < ns ? wanted : ns;
            }
        }

        //Limit all rates to 100Hz max. 100Hz = 10ms = 10000000ns
        if (wanted < 10000000LLU) {
            wanted = 10000000LLU;
        }

        int rate = ((wanted) / 5000000LLU) - ((wanted % 5000000LLU == 0) ? 1
                                                                         : 0); //mpu fifo rate is in increments of 5ms
        if (rate == 0) //KLP disallow fifo rate 0
            rate = 1;

        if (rate != mCurFifoRate) {
            ALOGD("set fifo rate: %d %llu", rate, wanted);
            inv_error_t res; // = inv_dmp_stop();
            res = inv_set_fifo_rate(rate);
            ALOGE_IF(res != INV_SUCCESS, "error setting FIFO rate");

            //res = inv_dmp_start();
            ALOGE_IF(res != INV_SUCCESS, "error re-starting DMP");

            mCurFifoRate = rate;
            rv = (res == INV_SUCCESS);
        }
    }
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

/* return the current time in nanoseconds */
int64_t MPLSensor::now_ns(void)
{
    //FUNC_ALOG;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    //ALOGV("Time %lld", (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec);
    return (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
}

int MPLSensor::readEvents(sensors_event_t* data, int count)
{
    //VFUNC_ALOG;
    int i;
    bool irq_set[5] = { false, false, false, false, false };
    inv_error_t rv;
    if (count < 1)
        return -EINVAL;
    int numEventReceived = 0;

    clearIrqData(irq_set);

    pthread_mutex_lock(&mMplMutex);
    if (mDmpStarted) {
        ALOGV_IF(EXTRA_VERBOSE, "Update Data");
        rv = inv_update_data();
        ALOGE_IF(rv != INV_SUCCESS, "inv_update_data error (code %d)", (int) rv);
    }
    else {
        //probably just one extra read after shutting down
        ALOGV_IF(EXTRA_VERBOSE,
                "MPLSensor::readEvents called, but there's nothing to do.");
    }
    pthread_mutex_unlock(&mMplMutex);

    if (!mNewData) {
        ALOGV_IF(EXTRA_VERBOSE, "no new data");
        return 0;
    }
    mNewData = 0;

    /* google timestamp */
    pthread_mutex_lock(&mMplMutex);
    for (int i = 0; i < numSensors; i++) {
        if (mEnabled & (1 << i)) {
            CALL_MEMBER_FN(this,mHandlers[i])(mPendingEvents + i,
                                              &mPendingMask, i);
            mPendingEvents[i].timestamp = irq_timestamp;
        }
    }

    for (int j = 0; count && mPendingMask && j < numSensors; j++) {
        if (mPendingMask & (1 << j)) {
            mPendingMask &= ~(1 << j);
            if (mEnabled & (1 << j)) {
                *data++ = mPendingEvents[j];
                count--;
                numEventReceived++;
            }
        }

    }

    pthread_mutex_unlock(&mMplMutex);
    return numEventReceived;
}

int MPLSensor::getFd() const
{
    ALOGV_IF(EXTRA_VERBOSE, "MPLSensor::getFd returning %d", data_fd);
    return data_fd;
}

int MPLSensor::getPollTime()
{
    return mPollTime;
}

bool MPLSensor::hasPendingEvents() const
{
    //if we are using the polling workaround, force the main loop to check for data every time
    return (mPollTime != -1);
}
