/* AudioStreamInALSA.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

namespace android
{

AudioStreamInALSA::AudioStreamInALSA(AudioHardwareALSA *parent,
        alsa_handle_t *handle,
        AudioSystem::audio_in_acoustics audio_acoustics) :
    ALSAStreamOps(parent, handle),
    mFramesLost(0),
    mAcoustics(audio_acoustics)
{
    acoustic_device_t *aDev = acoustics();

    if (aDev) aDev->set_params(aDev, mAcoustics, NULL);
}

AudioStreamInALSA::~AudioStreamInALSA()
{
    close();
}

status_t AudioStreamInALSA::setGain(float gain)
{
    return mixer() ? mixer()->setMasterGain(gain) : (status_t)NO_INIT;
}

ssize_t AudioStreamInALSA::read(void *buffer, ssize_t bytes)
{
    AutoMutex lock(mLock);

    if (!mPowerLock) {
        acquire_wake_lock (PARTIAL_WAKE_LOCK, "AudioInLock");
        mPowerLock = true;
    }

    acoustic_device_t *aDev = acoustics();

    // If there is an acoustics module read method, then it overrides this
    // implementation (unlike AudioStreamOutALSA write).
    if (aDev && aDev->read)
        return aDev->read(aDev, buffer, bytes);

    snd_pcm_sframes_t n, frames = snd_pcm_bytes_to_frames(mHandle->handle, bytes);
    status_t          err;

    do {
        n = snd_pcm_readi(mHandle->handle, buffer, frames);
        if (n < frames) {
            if (mHandle->handle) {
                if (n < 0) {
                    n = snd_pcm_recover(mHandle->handle, n, 0);
                    if (aDev && aDev->recover) aDev->recover(aDev, n);
                } else
                    n = snd_pcm_prepare(mHandle->handle);
            }
            return static_cast<ssize_t>(n);
        }
    } while (n == -EAGAIN);

    return static_cast<ssize_t>(snd_pcm_frames_to_bytes(mHandle->handle, n));
}

status_t AudioStreamInALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}

status_t AudioStreamInALSA::open(int mode)
{
    AutoMutex lock(mLock);

    status_t status = ALSAStreamOps::open(mode);

    acoustic_device_t *aDev = acoustics();

    if (status == NO_ERROR && aDev)
        status = aDev->use_handle(aDev, mHandle);

    return status;
}

status_t AudioStreamInALSA::close()
{
    AutoMutex lock(mLock);

    acoustic_device_t *aDev = acoustics();

    if (mHandle && aDev) aDev->cleanup(aDev);

    ALSAStreamOps::close();

    if (mPowerLock) {
        release_wake_lock ("AudioInLock");
        mPowerLock = false;
    }

    return NO_ERROR;
}

status_t AudioStreamInALSA::standby()
{
    AutoMutex lock(mLock);

    if (mPowerLock) {
        release_wake_lock ("AudioInLock");
        mPowerLock = false;
    }

    return NO_ERROR;
}

void AudioStreamInALSA::resetFramesLost()
{
    AutoMutex lock(mLock);
    mFramesLost = 0;
}

unsigned int AudioStreamInALSA::getInputFramesLost() const
{
    unsigned int count = mFramesLost;
    // Stupid interface wants us to have a side effect of clearing the count
    // but is defined as a const to prevent such a thing.
    ((AudioStreamInALSA *)this)->resetFramesLost();
    return count;
}

status_t AudioStreamInALSA::setAcousticParams(void *params)
{
    AutoMutex lock(mLock);

    acoustic_device_t *aDev = acoustics();

    return aDev ? aDev->set_params(aDev, mAcoustics, params) : (status_t)NO_ERROR;
}

}       // namespace android
