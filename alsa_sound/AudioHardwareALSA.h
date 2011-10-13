/* AudioHardwareALSA.h
 **
 ** Copyright 2008-2009, Wind River Systems
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

#ifndef ANDROID_AUDIO_HARDWARE_ALSA_H
#define ANDROID_AUDIO_HARDWARE_ALSA_H

#include <utils/List.h>
#include <hardware_legacy/AudioHardwareBase.h>

#include <alsa/asoundlib.h>

#include <hardware/hardware.h>

namespace android
{

class AudioHardwareALSA;

/**
 * The id of ALSA module
 */
#define ALSA_HARDWARE_MODULE_ID "alsa"
#define ALSA_HARDWARE_NAME      "alsa"

struct alsa_device_t;

struct alsa_handle_t {
    alsa_device_t *     module;
    uint32_t            devices;
    uint32_t            curDev;
    int                 curMode;
    snd_pcm_t *         handle;
    snd_pcm_format_t    format;
    uint32_t            channels;
    uint32_t            sampleRate;
    uint32_t		realsampleRate;
    unsigned int        latency;         // Delay in usec
    unsigned int        bufferSize;      // Size of sample buffer
    int                 mmap;
    int			interleaved;
    void *              modPrivate;
    unsigned		period_time;
    snd_pcm_uframes_t	period_frames;
    int			id;
    int			chunk_bytes;
};

typedef List<alsa_handle_t*> ALSAHandleList;

struct alsa_device_t {
    hw_device_t common;

    status_t (*init)(alsa_device_t *, ALSAHandleList &);
    status_t (*open)(alsa_handle_t *, uint32_t, int);
    status_t (*close)(alsa_handle_t *);
    status_t (*standby)(alsa_handle_t *);
    status_t (*route)(alsa_handle_t *, uint32_t, int);
    status_t (*voicevolume)(float);
    status_t (*set)(const String8&);
    status_t (*resetDefaults)(alsa_handle_t *handle);
};

/**
 * The id of acoustics module
 */
#define ACOUSTICS_HARDWARE_MODULE_ID    "acoustics"
#define ACOUSTICS_HARDWARE_NAME         "acoustics"

struct acoustic_device_t {
    hw_device_t common;

    // Required methods...
    status_t (*use_handle)(acoustic_device_t *, alsa_handle_t *);
    status_t (*cleanup)(acoustic_device_t *);

    status_t (*set_params)(acoustic_device_t *, AudioSystem::audio_in_acoustics, void *);

    // Optional methods...
    ssize_t (*read)(acoustic_device_t *, void *, size_t);
    ssize_t (*write)(acoustic_device_t *, const void *, size_t);
    status_t (*recover)(acoustic_device_t *, int);

    void *              modPrivate;
};

// ----------------------------------------------------------------------------

class ALSAMixer
{
public:
    ALSAMixer();
    virtual                ~ALSAMixer();

    bool                    isValid() { return !!mMixer[SND_PCM_STREAM_PLAYBACK]; }
    status_t                setMasterVolume(float volume);
    status_t                setMasterGain(float gain);

    status_t                setVolume(uint32_t device, float left, float right);

    status_t                setGain(uint32_t device, float gain);

    status_t                setCaptureMuteState(uint32_t device, bool state);
    status_t                getCaptureMuteState(uint32_t device, bool *state);
    status_t                setPlaybackMuteState(uint32_t device, bool state);
    status_t                getPlaybackMuteState(uint32_t device, bool *state);

private:
    snd_mixer_t *           mMixer[SND_PCM_STREAM_LAST+1];

};

class ALSAControl
{
public:
    ALSAControl(const char *device = "hw:00");
    virtual                ~ALSAControl();

    status_t                get(const char *name, unsigned int &value, int index = 0);
    status_t                set(const char *name, unsigned int value, int index = -1);

    status_t                set(const char *name, const char *);

    status_t                getmin(const char *name, unsigned int &max);
    status_t                getmax(const char *name, unsigned int &min);

private:
    snd_ctl_t *             mHandle;
};

class ALSAStreamOps
{
public:
    ALSAStreamOps(AudioHardwareALSA *parent, alsa_handle_t *handle);
    virtual            ~ALSAStreamOps();

    status_t            set(int *format, uint32_t *channels, uint32_t *rate);

    status_t            setParameters(const String8& keyValuePairs);
    String8             getParameters(const String8& keys);

    uint32_t            sampleRate() const;
    size_t              bufferSize() const;
    int                 format() const;
    uint32_t            channels() const;

    status_t            open(int mode);
    void                close();

protected:
    friend class AudioHardwareALSA;

    acoustic_device_t *acoustics();
    ALSAMixer *mixer();

    AudioHardwareALSA *     mParent;
    alsa_handle_t *         mHandle;

    Mutex                   mLock;
    bool                    mPowerLock;

    unsigned int	    mSamplerate;
    unsigned int	    mChannels;
    unsigned int	    mFormat;
};

// ----------------------------------------------------------------------------

class AudioStreamOutALSA : public AudioStreamOut, public ALSAStreamOps
{
public:
    AudioStreamOutALSA(AudioHardwareALSA *parent, alsa_handle_t *handle);
    virtual            ~AudioStreamOutALSA();

    virtual uint32_t    sampleRate() const
    {
        return ALSAStreamOps::sampleRate();
    }

    virtual size_t      bufferSize() const
    {
        return ALSAStreamOps::bufferSize();
    }

    virtual uint32_t    channels() const;

    virtual int         format() const
    {
        return ALSAStreamOps::format();
    }

    virtual uint32_t    latency() const;

    virtual ssize_t     write(const void *buffer, size_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);

    status_t            setVolume(float left, float right);

    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs) {
        return ALSAStreamOps::setParameters(keyValuePairs);
    }

    virtual String8     getParameters(const String8& keys) {
        return ALSAStreamOps::getParameters(keys);
    }

    // return the number of audio frames written by the audio dsp to DAC since
    // the output has exited standby
    virtual status_t    getRenderPosition(uint32_t *dspFrames);

    status_t            open(int mode);
    status_t            close();

private:
    uint32_t            mFrameCount;

};

class AudioStreamInALSA : public AudioStreamIn, public ALSAStreamOps
{
public:
    AudioStreamInALSA(AudioHardwareALSA *parent,
            alsa_handle_t *handle,
            AudioSystem::audio_in_acoustics audio_acoustics);
    virtual            ~AudioStreamInALSA();

    virtual uint32_t    sampleRate() const
    {
        return ALSAStreamOps::sampleRate();
    }

    virtual size_t      bufferSize() const
    {
        return ALSAStreamOps::bufferSize();
    }

    virtual uint32_t    channels() const
    {
        return ALSAStreamOps::channels();
    }

    virtual int         format() const
    {
        return ALSAStreamOps::format();
    }

    virtual ssize_t     read(void* buffer, ssize_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);

    virtual status_t    setGain(float gain);

    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs)
    {
        return ALSAStreamOps::setParameters(keyValuePairs);
    }

    virtual String8     getParameters(const String8& keys)
    {
        return ALSAStreamOps::getParameters(keys);
    }

    // Return the amount of input frames lost in the audio driver since the last call of this function.
    // Audio driver is expected to reset the value to 0 and restart counting upon returning the current value by this function call.
    // Such loss typically occurs when the user space process is blocked longer than the capacity of audio driver buffers.
    // Unit: the number of input audio frames
    virtual unsigned int  getInputFramesLost() const;

    status_t            setAcousticParams(void* params);

    status_t            open(int mode);
    status_t            close();

private:
    void                resetFramesLost();

    unsigned int        mFramesLost;
    AudioSystem::audio_in_acoustics mAcoustics;
};

class AudioHardwareALSA : public AudioHardwareBase
{
public:
    AudioHardwareALSA();
    virtual            ~AudioHardwareALSA();

    /**
     * check to see if the audio hardware interface has been initialized.
     * return status based on values defined in include/utils/Errors.h
     */
    virtual status_t    initCheck();

    /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
    virtual status_t    setVoiceVolume(float volume);

    /**
     * set the audio volume for all audio activities other than voice call.
     * Range between 0.0 and 1.0. If any value other than NO_ERROR is returned,
     * the software mixer will emulate this capability.
     */
    virtual status_t    setMasterVolume(float volume);

    /**
     * setMode is called when the audio mode changes. NORMAL mode is for
     * standard audio playback, RINGTONE when a ringtone is playing, and IN_CALL
     * when a call is in progress.
     */
    virtual status_t    setMode(int mode);

    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);

    // set/get global audio parameters
    virtual status_t    setParameters(const String8& keyValuePairs);

    //virtual String8     getParameters(const String8& keys);

    // Returns audio input buffer size according to parameters passed or 0 if one of the
    // parameters is not supported
    virtual size_t    getInputBufferSize(uint32_t sampleRate, int format, int channels);

    /** This method creates and opens the audio hardware output stream */
    virtual AudioStreamOut* openOutputStream(
            uint32_t devices,
            int *format=0,
            uint32_t *channels=0,
            uint32_t *sampleRate=0,
            status_t *status=0);
    virtual    void        closeOutputStream(AudioStreamOut* out);

    /** This method creates and opens the audio hardware input stream */
    virtual AudioStreamIn* openInputStream(
            uint32_t devices,
            int *format,
            uint32_t *channels,
            uint32_t *sampleRate,
            status_t *status,
            AudioSystem::audio_in_acoustics acoustics);
    virtual    void        closeInputStream(AudioStreamIn* in);

    /**This method dumps the state of the audio hardware */
    //virtual status_t dumpState(int fd, const Vector<String16>& args);

    static AudioHardwareInterface* create();

    int                 mode()
    {
        return mMode;
    }

protected:
    virtual status_t    dump(int fd, const Vector<String16>& args);

    friend class AudioStreamOutALSA;
    friend class AudioStreamInALSA;
    friend class ALSAStreamOps;

    ALSAMixer *         mMixer;

    alsa_device_t *     mALSADevice;
    acoustic_device_t * mAcousticDevice;

    ALSAHandleList      mDeviceList;

private:
    Mutex               mLock;
};

// ----------------------------------------------------------------------------

};        // namespace android
#endif    // ANDROID_AUDIO_HARDWARE_ALSA_H
