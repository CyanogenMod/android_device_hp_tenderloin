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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sound/asound.h>
#include <linux/uinput.h>
#include <pthread.h>

#include "AudioHardwareALSA.h"
#include <media/AudioRecord.h>

//#define IDLE_CONTROL

#undef DISABLE_HARWARE_RESAMPLING

#define ALSA_NAME_MAX 128

#define ALSA_STRCAT(x,y) \
    if (strlen(x) + strlen(y) < ALSA_NAME_MAX) \
        strcat(x, y);

#ifndef ALSA_DEFAULT_SAMPLE_RATE
#define ALSA_DEFAULT_SAMPLE_RATE 44100 // in Hz
#endif

struct snd_ctl_elem_id {
	unsigned int numid;		/* numeric identifier, zero = invalid */
	snd_ctl_elem_iface_t iface;	/* interface identifier */
	unsigned int device;		/* device/client number */
	unsigned int subdevice;		/* subdevice (substream) number */
	unsigned char name[44];		/* ASCII name of item */
	unsigned int index;		/* index of item */
};

struct snd_ctl_elem_value {
	struct snd_ctl_elem_id id;	/* W: element ID */
	unsigned int indirect: 1;	/* W: indirect access - obsoleted */
	union {
		union {
			long value[128];
			long *value_ptr;	/* obsoleted */
		} integer;
		union {
			long long value[64];
			long long *value_ptr;	/* obsoleted */
		} integer64;
		union {
			unsigned int item[128];
			unsigned int *item_ptr;	/* obsoleted */
		} enumerated;
		union {
			unsigned char data[512];
			unsigned char *data_ptr;	/* obsoleted */
		} bytes;
		struct snd_aes_iec958 iec958;
	} value;		/* RO */
	struct timespec tstamp;
	unsigned char reserved[128-sizeof(struct timespec)];
};

struct snd_ctl_elem_list {
	unsigned int offset;		/* W: first element ID to get */
	unsigned int space;		/* W: count of element IDs to get */
	unsigned int used;		/* R: count of element IDs set */
	unsigned int count;		/* R: count of all elements */
	struct snd_ctl_elem_id __user *pids; /* R: IDs */
	unsigned char reserved[50];
};

namespace android
{

struct snd_ctl_elem_id *elements;
int nelements, idle0_standalone_fd, idle0_collapse_fd, 
    idle1_standalone_fd, idle1_collapse_fd;

static struct dsp_control {
    int fd;
    struct snd_ctl_elem_id *pcm_playback_id;
    struct snd_ctl_elem_id *pcm_capture_id;
    struct snd_ctl_elem_id *speaker_stereo_rx_id;
    struct snd_ctl_elem_id *speaker_mono_tx_id;
    struct snd_ctl_elem_id *line1outn_id;
    struct snd_ctl_elem_id *line1outp_id;
    struct snd_ctl_elem_id *line2outn_id;
    struct snd_ctl_elem_id *line2outp_id;
    struct snd_ctl_elem_id *outvol_id;
} dsp;

int get_elem_list( int fd, struct snd_ctl_elem_list* list )
{
    int i = 0;
    struct snd_ctl_elem_id* pid;
    if (ioctl( fd, SNDRV_CTL_IOCTL_ELEM_LIST, list) < 0) {
        printf( " SNDRV_CTL_IOCTL_ELEM_LIST fail\n" );
        return -1;
    }

    return 0;
}

struct snd_ctl_elem_id* get_id(char *name)
{
	int i;
	for( i = 0; i < nelements; i++ ) {
		if(strcmp((const char*)elements[i].name,name)==0)
			return &elements[i];
	}
	printf("Error unable to locate mixer control: %s\n", name);
	return 0;
}

int write_elem(int fd, struct snd_ctl_elem_id* id, int d0, int d1, int d2)
{
    struct snd_ctl_elem_value control;

	control.id = *id;
	control.value.integer.value[0] = d0;
	control.value.integer.value[1] = d1;
	control.value.integer.value[2] = d2;
    if (ioctl( fd,SNDRV_CTL_IOCTL_ELEM_WRITE, &control ) < 0) {
		printf( " SNDRV_CTL_IOCTL_ELEM_WRITE fail\n");
		return -1;
	}

	return 0;
}

// ----------------------------------------------------------------------------

static int s_device_open(const hw_module_t*, const char*, hw_device_t**);
static int s_device_close(hw_device_t*);
static status_t s_init(alsa_device_t *, ALSAHandleList &);
static status_t s_open(alsa_handle_t *, uint32_t, int);
static status_t s_close(alsa_handle_t *);
static status_t s_standby(alsa_handle_t *);
static status_t s_route(alsa_handle_t *, uint32_t, int);
static status_t s_resetDefaults(alsa_handle_t *handle);

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
    dev->resetDefaults = s_resetDefaults;

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
    realsampleRate : 0,
    latency     : 0, // Desired Delay in usec
    bufferSize  : 0, // Desired Number of samples
    mmap        : 0,
    interleaved : 1,
    modPrivate  : 0,
    period_time : 0,
    period_frames: 1024,
    id		: 0,
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
    realsampleRate : 0,
    latency     : 0, // Desired Delay in usec
    bufferSize  : 0, // Desired Number of samples
    mmap        : 0,
    interleaved : 1,
    modPrivate  : 0,
    period_time : 0,
    period_frames: 0,
    id		: 1,
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

	if(handle==&_defaultsOut)
	{
		if(_defaultsIn.handle)
		{
			LOGE("Opening output device but input already running!");
			handle->sampleRate = _defaultsIn.realsampleRate;
		}
	}

	if(handle==&_defaultsIn)
	{
		if(_defaultsOut.handle)
		{
			LOGE("Opening input device but output already running!");
			handle->sampleRate = _defaultsOut.realsampleRate;
		}
	}

	
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

	handle->realsampleRate = handle->sampleRate;

//	buffer_frames = buffer_size;	/* for position test */
	return NO_ERROR;
}

// ----------------------------------------------------------------------------

void* headphone_thread(void*)
{
    int fd = open("/dev/input/event5", O_RDONLY);
    if (fd <= 0) {
        LOGE("Error opening event file for headphone detection");
        return 0;
    }
    struct input_event ev;

    while (1)
    {
	    read(fd, &ev, sizeof(struct input_event));

        if(ev.type == 5) {
	        if(ev.value)
            {
                LOGI("Headphones enabled");
                write_elem(dsp.fd,dsp.outvol_id,0x75,0x75,0);
                write_elem(dsp.fd, dsp.line1outn_id, 0, 0, 0);
                write_elem(dsp.fd, dsp.line1outp_id, 0, 0, 0);
                write_elem(dsp.fd, dsp.line2outn_id, 0, 0, 0);
                write_elem(dsp.fd, dsp.line2outp_id, 0, 0, 0);
            } else {
                LOGI("Headphones disabled");
                write_elem(dsp.fd, dsp.line1outn_id, 1, 1, 1);
                write_elem(dsp.fd, dsp.line1outp_id, 1, 1, 1);
                write_elem(dsp.fd, dsp.line2outn_id, 1, 1, 1);
                write_elem(dsp.fd, dsp.line2outp_id, 1, 1, 1);
                write_elem(dsp.fd,dsp.outvol_id,0x3B,0x3B,0);
            }
        }
    }
    return 0;
}

static status_t s_init(alsa_device_t *module, ALSAHandleList &list)
{
    pthread_t thread;

    struct snd_ctl_elem_list elem_list;
    struct snd_ctl_elem_id *line1mix_id;
    struct snd_ctl_elem_id *line2mix_id;
    struct snd_ctl_elem_id *leftdacmix_id;
    struct snd_ctl_elem_id *rightdacmix_id;
    struct snd_ctl_elem_id *aif2adc_id;
    struct snd_ctl_elem_id *aif2adcvol_id;
    struct snd_ctl_elem_id *aif2adcr_id;
    struct snd_ctl_elem_id *aif2dacl_id;
    struct snd_ctl_elem_id *dac2vol_id;
    struct snd_ctl_elem_id *in1pgan_id;
    struct snd_ctl_elem_id *in1pgap_id;
    struct snd_ctl_elem_id *mixinl_id;
    struct snd_ctl_elem_id *in1l_id;
    struct snd_ctl_elem_id *in1lvol_id;
    struct snd_ctl_elem_id *in1lzc_id;
    struct snd_ctl_elem_id *mixinl1_id;
    struct snd_ctl_elem_id *mixinlvol_id;
    struct snd_ctl_elem_id *dac1aifl_id;
    struct snd_ctl_elem_id *dac1aifr_id;
    struct snd_ctl_elem_id *dac1_id;
    struct snd_ctl_elem_id *dac2_id;

    dsp.fd = open("/dev/snd/controlC0", O_RDONLY);
    if(dsp.fd <=0) {
        LOGE("Unable to open sound device for init");
        exit(-1);
    }
#ifdef IDLE_CONTROL
    idle0_collapse_fd = open("/sys/module/pm_8x60/modes/cpu0/power_collapse/idle_enabled", O_WRONLY);
    idle0_standalone_fd = open("/sys/module/pm_8x60/modes/cpu0/standalone_power_collapse/idle_enabled", O_WRONLY);
    idle1_collapse_fd = open("/sys/module/pm_8x60/modes/cpu1/power_collapse/idle_enabled", O_WRONLY);
    idle1_standalone_fd = open("/sys/module/pm_8x60/modes/cpu1/standalone_power_collapse/idle_enabled", O_WRONLY);

    //Ignore open errors
    if(idle0_collapse_fd <= 0)
        LOGE("Unable to open cpu0/power_collapse/idle_enabled");
    if(idle0_standalone_fd <= 0)
        LOGE("Unable to open cpu0/standalone_power_collapse/idle_enabled");
    if(idle1_collapse_fd <= 0)
        LOGE("Unable to open cpu1/power_collapse/idle_enabled");
    if(idle1_standalone_fd <= 0)
        LOGE("Unable to open cpu1/standalone_power_collapse/idle_enabled");
#endif
    elem_list.offset = 0;
    elem_list.space  = 300;
    elements = ( struct snd_ctl_elem_id* ) malloc( elem_list.space * sizeof( struct snd_ctl_elem_id ) );
    elem_list.pids = elements;
    get_elem_list(dsp.fd,&elem_list);
    nelements = elem_list.used;

    dsp.pcm_playback_id = get_id("PCM Playback Sink");
    dsp.pcm_capture_id = get_id("PCM Capture Source");
    dsp.speaker_stereo_rx_id = get_id("speaker_stereo_rx");
    dsp.speaker_mono_tx_id = get_id("speaker_mono_tx");
    line1mix_id = get_id("LINEOUT1 Mixer Output Switch");
    dsp.line1outn_id = get_id("LINEOUT1N Switch");
    dsp.line1outp_id = get_id("LINEOUT1P Switch");
    line2mix_id = get_id("LINEOUT2 Mixer Output Switch");
    dsp.line2outn_id = get_id("LINEOUT2N Switch");
    dsp.line2outp_id = get_id("LINEOUT2P Switch");
    leftdacmix_id = get_id("Left Output Mixer DAC Switch");
    rightdacmix_id = get_id("Right Output Mixer DAC Switch");
    aif2adc_id = get_id("AIF2ADC HPF Switch");
    aif2adcvol_id = get_id("AIF2ADC Volume");
    aif2adcr_id = get_id("AIF2ADCR Source");
    aif2dacl_id = get_id("AIF2DAC2L Mixer Left Sidetone Switch");
    dac2vol_id = get_id("DAC2 Left Sidetone Volume");
    in1pgan_id = get_id("IN1L PGA IN1LN Switch");
    in1pgap_id = get_id("IN1L PGA IN1LP Switch");
    mixinl_id = get_id("MIXINL Output Record Volume");
    in1l_id = get_id("IN1L Switch");
    in1lvol_id = get_id("IN1L Volume");
    in1lzc_id = get_id("IN1L ZC Switch");
    mixinl1_id = get_id("MIXINL IN1L Switch");
    mixinlvol_id = get_id("MIXINL IN1L Volume");
    dsp.outvol_id = get_id("Output Volume");
    dac1aifl_id = get_id("DAC1L Mixer AIF1.1 Switch");
    dac1aifr_id = get_id("DAC1R Mixer AIF1.1 Switch");
    dac1_id = get_id("DAC1 Switch");
    dac2_id = get_id("DAC2 Switch");

    write_elem(dsp.fd,line1mix_id,1,0,0);
    write_elem(dsp.fd,dsp.line1outn_id,1,0,0);
    write_elem(dsp.fd,dsp.line1outp_id,1,0,0);

    write_elem(dsp.fd,line2mix_id,1,0,0);
    write_elem(dsp.fd,dsp.line2outn_id,1,0,0);
    write_elem(dsp.fd,dsp.line2outp_id,1,0,0);

    write_elem(dsp.fd,leftdacmix_id,1,0,0);
    write_elem(dsp.fd,rightdacmix_id,1,0,0);
    write_elem(dsp.fd,aif2adc_id,1,1,0);
    write_elem(dsp.fd,aif2adcvol_id,0x64,0x64,0);
    write_elem(dsp.fd,aif2adcr_id,0,0,0);
    write_elem(dsp.fd,aif2dacl_id,1,0,0);
    write_elem(dsp.fd,dac2vol_id,0xC,0,0);

    write_elem(dsp.fd,in1pgan_id,1,0,0);
    write_elem(dsp.fd,in1pgap_id,1,0,0);
    write_elem(dsp.fd,mixinl_id,1,0,0);
    write_elem(dsp.fd,in1l_id,1,0,0);
    write_elem(dsp.fd,in1lvol_id,0x1B,0,0);
    write_elem(dsp.fd,in1lzc_id,0,0,0);

    write_elem(dsp.fd,mixinl1_id,1,0,0);
    write_elem(dsp.fd,mixinlvol_id,0,0,0);
    write_elem(dsp.fd,dsp.outvol_id,0x3B,0x3B,0);
    write_elem(dsp.fd,dac1aifl_id,1,0,0);
    write_elem(dsp.fd,dac1aifr_id,1,0,0);

    write_elem(dsp.fd,dac1_id,1,1,0);
    write_elem(dsp.fd,dac2_id,1,1,0);

    list.clear();

    snd_pcm_uframes_t bufferSize = _defaultsOut.bufferSize;

    for (size_t i = 1; (bufferSize & ~i) != 0; i <<= 1)
        bufferSize &= ~i;

    _defaultsOut.module = module;
    _defaultsOut.bufferSize = bufferSize;

    list.push_back(&_defaultsOut);

    bufferSize = _defaultsIn.bufferSize;

    for (size_t i = 1; (bufferSize & ~i) != 0; i <<= 1)
        bufferSize &= ~i;

    _defaultsIn.module = module;
    _defaultsIn.bufferSize = bufferSize;

    list.push_back(&_defaultsIn);

    pthread_create (&thread, NULL, &headphone_thread, NULL);

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

    if (handle == &_defaultsOut) {
        write_elem(dsp.fd, dsp.pcm_playback_id, 0, 0, 1);
        write_elem(dsp.fd, dsp.speaker_stereo_rx_id, 1, 1, 1);
    }
    if (handle == &_defaultsIn) {
        write_elem(dsp.fd, dsp.pcm_capture_id, 0, 1, 1);
        write_elem(dsp.fd, dsp.speaker_mono_tx_id,1,1,1);
    }
#ifdef IDLE_CONTROL
    if(idle0_standalone_fd > 0) {
        int bytes = 0;
        bytes = write(idle0_standalone_fd, "0", 1);
        LOGI("Disable: Bytes written to standalone idle0_enabled: %d\n", bytes);
    }
    if(idle0_collapse_fd > 0) {
        int bytes = 0;
        bytes = write(idle0_collapse_fd, "0", 1);
        LOGI("Disable: Bytes written to collapse idle0_enabled: %d\n", bytes);
    }
    if(idle1_standalone_fd > 0) {
        int bytes = 0;
        bytes = write(idle1_standalone_fd, "0", 1);
        LOGI("Disable: Bytes written to standalone idle1_enabled: %d\n", bytes);
    }
    if(idle1_collapse_fd > 0) {
        int bytes = 0;
        bytes = write(idle1_collapse_fd, "0", 1);
        LOGI("Disable: Bytes written to collapse idle1_enabled: %d\n", bytes);
    }
#endif
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

    if (handle == &_defaultsOut) {
        write_elem(dsp.fd, dsp.pcm_playback_id, 0, 0, 0);
        write_elem(dsp.fd, dsp.speaker_stereo_rx_id, 0, 1, 0);
    }
    if (handle == &_defaultsIn) {
        write_elem(dsp.fd, dsp.pcm_capture_id, 0, 1, 0);
        write_elem(dsp.fd, dsp.speaker_mono_tx_id,0,1,0);
    }
#ifdef IDLE_CONTROL
    if (!_defaultsIn.handle && !_defaultsOut.handle) {
        if(idle0_standalone_fd > 0) {
            int bytes = 0;
            bytes = write(idle0_standalone_fd, "1", 1);
            LOGI("Enabled: Bytes written to standalone idle0_enabled: %d\n", bytes);
        }
        if(idle0_collapse_fd > 0) {
            int bytes = 0;
            bytes = write(idle0_collapse_fd, "1", 1);
            LOGI("Enabled: Bytes written to collapse idle0_enabled: %d\n", bytes);
        }
        if(idle1_standalone_fd > 0) {
            int bytes = 0;
            bytes = write(idle1_standalone_fd, "1", 1);
            LOGI("Enabled: Bytes written to standalone idle1_enabled: %d\n", bytes);
        }
        if(idle1_collapse_fd > 0) {
            int bytes = 0;
            bytes = write(idle1_collapse_fd, "1", 1);
            LOGI("Enabled: Bytes written to collapse idle1_enabled: %d\n", bytes);
        }
    }
#endif
    if(handle == &_defaultsIn)
        LOGI("ALSA Module: closing down input device");
    if(handle == &_defaultsOut)
        LOGI("ALSA Module: closing down output device");

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

static status_t s_resetDefaults(alsa_handle_t *handle)
{
	if(handle == &_defaultsIn)
		LOGE("Reset defaults called on input");
	if(handle == &_defaultsOut)
		LOGE("Reset defaults called on output");
	return NO_ERROR;
}

status_t setHardwareParams(alsa_handle_t *handle) {
    return 0;
}

status_t setSoftwareParams(alsa_handle_t *handle) {
    return 0;
}
}
