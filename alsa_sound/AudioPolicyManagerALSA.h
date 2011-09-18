/*
 * Copyright (C) 2009 The Android Open Source Project
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


#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <hardware_legacy/AudioPolicyManagerBase.h>


namespace android {

// ----------------------------------------------------------------------------

#define MAX_DEVICE_ADDRESS_LEN 20
// Attenuation applied to STRATEGY_SONIFICATION streams when a headset is connected: 6dB
#define SONIFICATION_HEADSET_VOLUME_FACTOR 0.5
// Min volume for STRATEGY_SONIFICATION streams when limited by music volume: -36dB
#define SONIFICATION_HEADSET_VOLUME_MIN  0.016
// Time in seconds during which we consider that music is still active after a music
// track was stopped - see computeVolume()
#define SONIFICATION_HEADSET_MUSIC_DELAY  5
class AudioPolicyManagerALSA: public AudioPolicyManagerBase
{

public:
                AudioPolicyManagerALSA(AudioPolicyClientInterface *clientInterface);
        virtual ~AudioPolicyManagerALSA();

        status_t setDeviceConnectionState(AudioSystem::audio_devices device,
                                                          AudioSystem::device_connection_state state,
                                                          const char *device_address);
        uint32_t getDeviceForStrategy(routing_strategy strategy, bool fromCache = true);
#ifdef HAS_FM_RADIO
        /* get Fm input source */
        audio_io_handle_t getFMInput(int inputSource,
                                            uint32_t samplingRate,
                                            uint32_t format,
                                            uint32_t channels,
                                            AudioSystem::audio_in_acoustics acoustics);
#endif
        /* AudioPolicyManagerBase.cpp, in the stopoutoutput, setOutputDevice
         * is called by force(with flag true) which is causing Core is not going
         * to off/ret when A2DP is paused, Because output stream is activated
         * during the A2DP pause (opens PCM stream).
         * So we are overriding the stopOutput in Alsa vesrion of policy manger
         * now setOutputDevice is called with flag false.
         * */
        status_t stopOutput(audio_io_handle_t output, AudioSystem::stream_type stream);
#ifdef HAS_FM_RADIO
        audio_io_handle_t mfmInput;       // FM input handler
#endif
};

};
