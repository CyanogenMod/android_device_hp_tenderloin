/* alsa_default.cpp
 **
 ** Copyright 2009 Wind River Systems
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

#define LOG_TAG "ALSAModule"
#include <utils/Log.h>

#include "AudioHardwareALSA.h"
#include <media/AudioRecord.h>

#undef DISABLE_HARWARE_RESAMPLING

#define ALSA_NAME_MAX 128

#define ALSA_STRCAT(x,y) \
    if (strlen(x) + strlen(y) < ALSA_NAME_MAX) \
        strcat(x, y);

#ifndef ALSA_DEFAULT_SAMPLE_RATE
#define ALSA_DEFAULT_SAMPLE_RATE 44100 // in Hz
#endif

namespace android
{

static int s_device_open(const hw_module_t*, const char*, hw_device_t**);
static int s_device_close(hw_device_t*);
static status_t s_init(alsa_device_t *, ALSAHandleList &);
static status_t s_open(alsa_handle_t *, uint32_t, int);
static status_t s_close(alsa_handle_t *);
static status_t s_standby(alsa_handle_t *);
static status_t s_route(alsa_handle_t *, uint32_t, int);

static hw_module_methods_t s_module_methods = {
    open            : s_device_open
};

extern "C" const hw_module_t HAL_MODULE_INFO_SYM = {
    tag             : HARDWARE_MODULE_TAG,
    version_major   : 1,
    version_minor   : 0,
    id              : ALSA_HARDWARE_MODULE_ID,
    name            : "ALSA module",
    author          : "Wind River",
    methods         : &s_module_methods,
    dso             : 0,
    reserved        : { 0, },
};

static int s_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    alsa_device_t *dev;
    dev = (alsa_device_t *) malloc(sizeof(*dev));
    if (!dev) return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (hw_module_t *) module;
    dev->common.close = s_device_close;
    dev->init = s_init;
    dev->open = s_open;
    dev->close = s_close;
    dev->standby = s_standby;
    dev->route = s_route;

    *device = &dev->common;
    return 0;
}

static int s_device_close(hw_device_t* device)
{
    free(device);
    return 0;
}

// ----------------------------------------------------------------------------

static const int DEFAULT_SAMPLE_RATE = ALSA_DEFAULT_SAMPLE_RATE;

static const char *devicePrefix[SND_PCM_STREAM_LAST + 1] = {
        /* SND_PCM_STREAM_PLAYBACK : */"AndroidPlayback",
        /* SND_PCM_STREAM_CAPTURE  : */"AndroidCapture",
};

static alsa_handle_t _defaultsOut = {
    module      : 0,
    devices     : AudioSystem::DEVICE_OUT_ALL,
    curDev      : 0,
    curMode     : 0,
    handle      : 0,
    format      : SND_PCM_FORMAT_S16_LE, // AudioSystem::PCM_16_BIT
    channels    : 2,
    sampleRate  : DEFAULT_SAMPLE_RATE,
    latency     : 0, // Desired Delay in usec
    bufferSize  : 0, // Desired Number of samples
    mmap        : 0,
    interleaved : 1,
    modPrivate  : 0,
    period_time : 0,
    period_frames: 1024,
};

static alsa_handle_t _defaultsIn = {
    module      : 0,
    devices     : AudioSystem::DEVICE_IN_ALL,
    curDev      : 0,
    curMode     : 0,
    handle      : 0,
    format      : SND_PCM_FORMAT_S16_LE, // AudioSystem::PCM_16_BIT
    channels    : 1,
    sampleRate  : AudioRecord::DEFAULT_SAMPLE_RATE,
    latency     : 0, // Desired Delay in usec
    bufferSize  : 0, // Desired Number of samples
    mmap        : 0,
    interleaved : 1,
    modPrivate  : 0,
    period_time : 0,
    period_frames: 0,

};

struct device_suffix_t {
    const AudioSystem::audio_devices device;
    const char *suffix;
};

/* The following table(s) need to match in order of the route bits
 */
static const device_suffix_t deviceSuffix[] = {
        {AudioSystem::DEVICE_OUT_EARPIECE,       "_Earpiece"},
        {AudioSystem::DEVICE_OUT_SPEAKER,        "_Speaker"},
        {AudioSystem::DEVICE_OUT_BLUETOOTH_SCO,  "_Bluetooth"},
        {AudioSystem::DEVICE_OUT_WIRED_HEADSET,  "_Headset"},
        {AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP, "_Bluetooth-A2DP"},
};

static const int deviceSuffixLen = (sizeof(deviceSuffix)
        / sizeof(device_suffix_t));

// ----------------------------------------------------------------------------

snd_pcm_stream_t direction(alsa_handle_t *handle)
{
    return (handle->devices & AudioSystem::DEVICE_OUT_ALL) ? SND_PCM_STREAM_PLAYBACK
            : SND_PCM_STREAM_CAPTURE;
}

const char *deviceName(alsa_handle_t *handle, uint32_t device, int mode)
{
    static char devString[ALSA_NAME_MAX];
    int hasDevExt = 0;

    strcpy(devString, devicePrefix[direction(handle)]);

    for (int dev = 0; device && dev < deviceSuffixLen; dev++)
        if (device & deviceSuffix[dev].device) {
            ALSA_STRCAT (devString, deviceSuffix[dev].suffix);
            device &= ~deviceSuffix[dev].device;
            hasDevExt = 1;
        }

    if (hasDevExt) switch (mode) {
    case AudioSystem::MODE_NORMAL:
        ALSA_STRCAT (devString, "_normal")
        ;
        break;
    case AudioSystem::MODE_RINGTONE:
        ALSA_STRCAT (devString, "_ringtone")
        ;
        break;
    case AudioSystem::MODE_IN_CALL:
        ALSA_STRCAT (devString, "_incall")
        ;
        break;
    };

    return devString;
}

const char *streamName(alsa_handle_t *handle)
{
    return snd_pcm_stream_name(direction(handle));
}

status_t setGlobalParams(alsa_handle_t *handle)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	int err;
	size_t n;
	unsigned int rate;
	int monotonic, can_pause, start_delay, stop_delay, bits_per_sample, bytes_per_frame, bits_per_frame;
	void* audiobuf=0;
	int avail_min = -1;
	snd_pcm_uframes_t start_threshold, stop_threshold, chunk_size;

	LOGI("Set global parms\n");

	usleep(1000);

	
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(handle->handle, params);


	if (err < 0) {
		LOGE("Broken configuration for this PCM: no configurations available");
		return NO_INIT;
	}
	if (handle->mmap) {
		LOGI("Setting mmap\n");

		snd_pcm_access_mask_t *mask = (snd_pcm_access_mask_t*)alloca(snd_pcm_access_mask_sizeof());
		snd_pcm_access_mask_none(mask);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
		err = snd_pcm_hw_params_set_access_mask(handle->handle, params, mask);
	} else if (handle->interleaved)
	{
		LOGI("Setting interleved PCM\n");
		err = snd_pcm_hw_params_set_access(handle->handle, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	}	
	else
	{	
		LOGI("Setting standard PCM");
		snd_pcm_access_mask_t *mask = (snd_pcm_access_mask_t*)alloca(snd_pcm_access_mask_sizeof());
		snd_pcm_access_mask_none(mask);
		err = snd_pcm_hw_params_set_access_mask(handle->handle, params, mask);
		err = snd_pcm_hw_params_set_access(handle->handle, params,
						   SND_PCM_ACCESS_RW_NONINTERLEAVED);
	}
	if (err < 0) {
		LOGE("Access type not available");
		return NO_INIT;
	}
	err = snd_pcm_hw_params_set_format(handle->handle, params, handle->format);
	if (err < 0) {
		LOGE("Sample format non available");
		//show_available_sample_formats(params);
		return NO_INIT;
	}
	err = snd_pcm_hw_params_set_channels(handle->handle, params, handle->channels);
	if (err < 0) {
		LOGE("Channels count non available");
		return NO_INIT;
	}

#if 0
	err = snd_pcm_hw_params_set_periods_min(handle, params, 2);
	assert(err >= 0);
#endif
	rate = handle->sampleRate;
	err = snd_pcm_hw_params_set_rate_near(handle->handle, params, &handle->sampleRate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < handle->sampleRate || (float)rate * 0.95 > handle->sampleRate) \
	{
		char plugex[64];
		const char *pcmname = snd_pcm_name(handle->handle);
		LOGE("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n", rate, handle->sampleRate);
		if (! pcmname || strchr(snd_pcm_name(handle->handle), ':'))
			*plugex = 0;
		else
	
			snprintf(plugex, sizeof(plugex), "(-Dplug:%s)",
					 snd_pcm_name(handle->handle));
		LOGE("         please, try the plug plugin %s\n",
			plugex);
	}
	rate = handle->sampleRate;
	if (handle->latency == 0 && handle->bufferSize == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
							    &handle->latency, 0);
		assert(err >= 0);
		if (handle->latency > 500000)
			handle->latency = 500000;
	}
	if (handle->period_time == 0 && handle->period_frames == 0) {
		if (handle->latency > 0)
			handle->period_time = handle->latency / 4;
		else
			handle->period_frames = handle->bufferSize / 4;
	}
	if (handle->period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle->handle, params,
							     &handle->period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle->handle, params,
							     &handle->period_frames, 0);
	assert(err >= 0);
	if (handle->latency > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle->handle, params,
							     &handle->latency, 0);
	} else {
		snd_pcm_uframes_t tframes = (snd_pcm_uframes_t)handle->bufferSize;
		err = snd_pcm_hw_params_set_buffer_size_near(handle->handle, params,
							     &tframes);
		handle->bufferSize = (int)tframes;
	}
	assert(err >= 0);
	monotonic = snd_pcm_hw_params_is_monotonic(params);
	can_pause = snd_pcm_hw_params_can_pause(params);
	err = snd_pcm_hw_params(handle->handle, params);
	if (err < 0) {
		LOGE("Unable to install hw params:");
		return NO_INIT;
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t*)&handle->bufferSize);
	if (chunk_size == handle->bufferSize) {
		LOGE("Can't use period equal to buffer size (%lu == %lu)",
		      chunk_size, handle->bufferSize);
		return NO_INIT;
	}

	snd_pcm_sw_params_current(handle->handle, swparams);
	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) rate * avail_min / 1000000;
	err = snd_pcm_sw_params_set_avail_min(handle->handle, swparams, n);

	/* round up to closest transfer boundary */
	n = handle->bufferSize;
	if (start_delay <= 0) {
		start_threshold = n + (double) rate * start_delay / 1000000;
	} else
		start_threshold = (double) rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(handle->handle, swparams, start_threshold);
	assert(err >= 0);
	if (stop_delay <= 0) 
		stop_threshold = handle->bufferSize + (double) rate * stop_delay / 1000000;
	else
		stop_threshold = (double) rate * stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(handle->handle, swparams, stop_threshold);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle->handle, swparams) < 0) {
		LOGE("unable to install sw params:");
		return NO_INIT;
	}

	bits_per_sample = snd_pcm_format_physical_width(handle->format);
	bits_per_frame = bits_per_sample * handle->channels;
	handle->chunk_bytes = chunk_size * bits_per_frame / 8;

	LOGI("Buffer size: %lu, chunk %d, latency %d", handle->bufferSize, handle->chunk_bytes, handle->latency);

/*	audiobuf = realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		LOGE("not enough memory");
		return NO_INIT;
	}*/
	// fprintf(stderr, "real chunk_size = %i, frags = %i, total = %i\n", chunk_size, setup.buf.block.frags, setup.buf.block.frags * chunk_size);

	/* show mmap buffer arragment */
	if (handle->mmap) {
		const snd_pcm_channel_area_t *areas;
		snd_pcm_uframes_t offset, size = chunk_size;
		int i;
		err = snd_pcm_mmap_begin(handle->handle, &areas, &offset, &size);
		if (err < 0) {
			LOGE("snd_pcm_mmap_begin problem: %s", snd_strerror(err));
			return NO_INIT;
		}
		for (i = 0; i < handle->channels; i++)
			LOGI("mmap_area[%i] = %p,%u,%u (%u)\n", i, areas[i].addr, areas[i].first, areas[i].step, snd_pcm_format_physical_width(handle->format));
		/* not required, but for sure */
		snd_pcm_mmap_commit(handle->handle, offset, 0);
	}

//	buffer_frames = buffer_size;	/* for position test */
	return NO_ERROR;
}

// ----------------------------------------------------------------------------

static status_t s_init(alsa_device_t *module, ALSAHandleList &list)
{
    list.clear();

    snd_pcm_uframes_t bufferSize = _defaultsOut.bufferSize;

    for (size_t i = 1; (bufferSize & ~i) != 0; i <<= 1)
        bufferSize &= ~i;

    _defaultsOut.module = module;
    _defaultsOut.bufferSize = bufferSize;

    list.push_back(_defaultsOut);

    bufferSize = _defaultsIn.bufferSize;

    for (size_t i = 1; (bufferSize & ~i) != 0; i <<= 1)
        bufferSize &= ~i;

    _defaultsIn.module = module;
    _defaultsIn.bufferSize = bufferSize;

    list.push_back(_defaultsIn);

    return NO_ERROR;
}

static status_t s_open(alsa_handle_t *handle, uint32_t devices, int mode)
{

 /*	if (handle->handle && handle->curDev == devices && handle->curMode == mode) 
	{
		LOGI("ALSA Module called open on already open device %08x in mode %d...", devices, mode); 
		return NO_ERROR;	
	}*/
    // Close off previously opened device.
    // It would be nice to determine if the underlying device actually
    // changes, but we might be recovering from an error or manipulating
    // mixer settings (see asound.conf).
    //
    s_close(handle);

    LOGD("open called for devices %08x in mode %d...", devices, mode);

    const char *stream = streamName(handle);
    const char *devName = deviceName(handle, devices, mode);

    int err;

    for (;;) {
        // The PCM stream is opened in blocking mode, per ALSA defaults.  The
        // AudioFlinger seems to assume blocking mode too, so asynchronous mode
        // should not be used.
        err = snd_pcm_open(&handle->handle, devName, direction(handle),
                SND_PCM_ASYNC);
        if (err == 0) break;

        // See if there is a less specific name we can try.
        // Note: We are changing the contents of a const char * here.
        char *tail = strrchr(devName, '_');
        if (!tail) break;
        *tail = 0;
    }

    if (err < 0) {
        // None of the Android defined audio devices exist. Open a generic one.
        devName = "default";
        err = snd_pcm_open(&handle->handle, devName, direction(handle), 0);
    }

    if (err < 0) {
        LOGE("Failed to Initialize any ALSA %s device: %s",
                stream, strerror(err));
        return NO_INIT;
    }

/*
    err = setHardwareParams(handle);

    if (err == NO_ERROR) err = setSoftwareParams(handle); */
    setGlobalParams(handle);

    LOGI("Initialized ALSA %s device %s", stream, devName);

    handle->curDev = devices;
    handle->curMode = mode;

    return err;
}

static status_t s_close(alsa_handle_t *handle)
{
    status_t err = NO_ERROR;
    snd_pcm_t *h = handle->handle;
    handle->handle = 0;
    handle->curDev = 0;
    handle->curMode = 0;
    if (h) {
        snd_pcm_drain(h);
        err = snd_pcm_close(h);
    }

	LOGI("ALSA Module: closing down device");

    return err;
}

static status_t s_standby(alsa_handle_t *handle)
{
    //hw specific modules may choose to implement
    //this differently to gain a power savings during
    //standby
    s_close(handle);
    handle->handle = 0;
    return NO_ERROR;

}

static status_t s_route(alsa_handle_t *handle, uint32_t devices, int mode)
{
    LOGD("route called for devices %08x in mode %d...", devices, mode);

    if (handle->handle && handle->curDev == devices && handle->curMode == mode) return NO_ERROR;

    return s_open(handle, devices, mode);
}

}
