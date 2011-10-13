/* AudioStreamOutALSA.cpp
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
#include <sys/time.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

#ifndef ALSA_DEFAULT_SAMPLE_RATE
#define ALSA_DEFAULT_SAMPLE_RATE 44100 // in Hz
#endif

namespace android
{

// ----------------------------------------------------------------------------

static const int DEFAULT_SAMPLE_RATE = ALSA_DEFAULT_SAMPLE_RATE;

// ----------------------------------------------------------------------------

AudioStreamOutALSA::AudioStreamOutALSA(AudioHardwareALSA *parent,
alsa_handle_t *handle) :
   ALSAStreamOps(parent, handle),
   mFrameCount(0)
{
}

AudioStreamOutALSA::~AudioStreamOutALSA()
{
   close();
}

uint32_t AudioStreamOutALSA::channels() const
{
   int c = ALSAStreamOps::channels();
   return c;
}

status_t AudioStreamOutALSA::setVolume(float left, float right)
{
   return mixer()->setVolume (mHandle->curDev, left, right);
}


char real_buffer[32 * 1024];
ssize_t AudioStreamOutALSA::write(const void *buffer, size_t bytes)
{
   AutoMutex lock(mLock);
   size_t i;

   //Make sure we are not causing ioctl's with bad buffers
  // for(i=0; i < bytes; i++)
       //real_buffer[i] = *(char*)buffer+i;

   if (!mPowerLock) {
       acquire_wake_lock (PARTIAL_WAKE_LOCK, "AudioOutLock");
       mPowerLock = true;
   }

       /* check if handle is still valid, otherwise we are coming out of standby */
       if(mHandle->handle == NULL) {
        nsecs_t previously = systemTime();
	//Attempt to restore the configuration
	mHandle->channels = mChannels;
  	mHandle->sampleRate = mSamplerate;
	mHandle->format = (snd_pcm_format_t)mFormat;

            mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);
        nsecs_t delta = systemTime() - previously;
        LOGE("RE-OPEN AFTER STANDBY:: took %llu msecs\n", ns2ms(delta));
       }

   acoustic_device_t *aDev = acoustics();

   // For output, we will pass the data on to the acoustics module, but the actual
   // data is expected to be sent to the audio device directly as well.
   if (aDev && aDev->write)
       aDev->write(aDev, buffer, bytes);

   snd_pcm_sframes_t n;
   size_t            sent = 0;
   status_t          err;

   do {
       // for hotpluggable devices (e.g. hdmi)
       if (!mHandle->handle)
                   return sent;

       if (mHandle->mmap) {
               LOGE("Tried to write MMAP Stream!");
/*            n = snd_pcm_mmap_writei(mHandle->handle,
                                  (char *)buffer + sent,

snd_pcm_bytes_to_frames(mHandle->handle, bytes - sent));*/
       }
       else {
  //    LOGE("waiting for send");
               int to_write = snd_pcm_bytes_to_frames(mHandle->handle, bytes - sent);
		int n_frames = to_write;
               int chunk_frames =  snd_pcm_bytes_to_frames(mHandle->handle, mHandle->chunk_bytes);

/*	if (count < chunk_size) {
		snd_pcm_format_set_silence(mHandle->format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
		count = chunk_size;
	} */
               if(to_write > chunk_frames)
                 to_write = chunk_frames;
           n = snd_pcm_writei(mHandle->handle,
                              (char *)buffer + sent,
                              chunk_frames);//to_write);


       //    n = snd_pcm_bytes_to_frames(mHandle->handle, bytes - sent);

   //   LOGE("would have sent %d frames, chunk %d, done %d", n_frames, chunk_frames, n);
       }
       if (n == -EBADFD) {
		LOGE("bad fd");
           // Somehow the stream is in a bad state. The driver probably
           // has a bug and snd_pcm_recover() doesn't seem to handle this.
           mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);

           if (aDev && aDev->recover) aDev->recover(aDev, n);
           //bail
           if (n) return static_cast<ssize_t>(n);
       }
       else if (n < 0 && n != -EAGAIN) {
           if (mHandle->handle) {
               snd_pcm_status_t *status;
   		snd_pcm_status_alloca(&status);
               if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN || 
			snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
                       snd_pcm_prepare(mHandle->handle); //Get it running again

               }else{

                       // snd_pcm_recover() will return 0 if successful in recovering from
                       // an error, or -errno if the error was unrecoverable.
                       n = snd_pcm_recover(mHandle->handle, n, 1);

                        if (aDev && aDev->recover) aDev->recover(aDev, n);
                               if (n) return static_cast<ssize_t>(n);
               }
           }
       }
       else {
           if (mHandle->handle) {
               mFrameCount += n;

               sent +=
static_cast<ssize_t>(snd_pcm_frames_to_bytes(mHandle->handle, n));

               if (n == -EAGAIN || n >= 0) {// && (size_t)sent < bytes)) {
               //      if (!test_nowait)
                               snd_pcm_wait(mHandle->handle, 100);
               }
           }
       }

   } while (mHandle->handle && sent < bytes);

   return sent;
}

status_t AudioStreamOutALSA::dump(int fd, const Vector<String16>& args)
{
   return NO_ERROR;
}

status_t AudioStreamOutALSA::open(int mode)
{
   AutoMutex lock(mLock);
   return ALSAStreamOps::open(mode);
}

status_t AudioStreamOutALSA::close()
{
   AutoMutex lock(mLock);

   snd_pcm_drain (mHandle->handle);
   ALSAStreamOps::close();

   if (mPowerLock) {
       release_wake_lock ("AudioOutLock");
       mPowerLock = false;
   }

   return NO_ERROR;
}

status_t AudioStreamOutALSA::standby()
{
   AutoMutex lock(mLock);

   if (mHandle->module->standby)
   // allow hw specific modules to imlement unique standby
   // if needed
       mHandle->module->standby(mHandle);
   else
       snd_pcm_drain (mHandle->handle);

   if (mPowerLock) {
       release_wake_lock ("AudioOutLock");
       mPowerLock = false;
   }

   mFrameCount = 0;

   return NO_ERROR;
}

#define USEC_TO_MSEC(x) ((x + 999) / 1000)

uint32_t AudioStreamOutALSA::latency() const
{
   // Android wants latency in milliseconds.
   return USEC_TO_MSEC (mHandle->latency);
}

// return the number of audio frames written by the audio dsp to DAC since
// the output has exited standby
status_t AudioStreamOutALSA::getRenderPosition(uint32_t *dspFrames)
{
   *dspFrames = mFrameCount;
   //*dspFrames = 0;

   return NO_ERROR;
}

}       // namespace android
