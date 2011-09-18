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

#define LOG_TAG "AudioPolicyManagerALSA"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include "AudioPolicyManagerALSA.h"
#include <media/mediarecorder.h>

namespace android {

status_t AudioPolicyManagerALSA::setDeviceConnectionState(AudioSystem::audio_devices device,
                                                  AudioSystem::device_connection_state state,
                                                  const char *device_address)
{

    LOGV("setDeviceConnectionState() device: %x, state %d, address %s", device, state, device_address);

    // connect/disconnect only 1 device at a time
    if (AudioSystem::popCount(device) != 1) return BAD_VALUE;

    if (strlen(device_address) >= MAX_DEVICE_ADDRESS_LEN) {
        LOGE("setDeviceConnectionState() invalid address: %s", device_address);
        return BAD_VALUE;
    }

    // handle output devices
    if (AudioSystem::isOutputDevice(device)) {

#ifndef WITH_A2DP
        if (AudioSystem::isA2dpDevice(device)) {
            LOGE("setDeviceConnectionState() invalid device: %x", device);
            return BAD_VALUE;
        }
#endif

        switch (state)
        {
        // handle output device connection
        case AudioSystem::DEVICE_STATE_AVAILABLE:
            if (mAvailableOutputDevices & device) {
                LOGW("setDeviceConnectionState() device already connected: %x", device);
                return INVALID_OPERATION;
            }
            LOGV("setDeviceConnectionState() connecting device %x", device);

            // register new device as available
            mAvailableOutputDevices |= device;

#ifdef WITH_A2DP
            // handle A2DP device connection
            if (AudioSystem::isA2dpDevice(device)) {
                status_t status = handleA2dpConnection(device, device_address);
                if (status != NO_ERROR) {
                    mAvailableOutputDevices &= ~device;
                    return status;
                }
            } else
#endif
            {
                if (AudioSystem::isBluetoothScoDevice(device)) {
                    LOGV("setDeviceConnectionState() BT SCO  device, address %s", device_address);
                    // keep track of SCO device address
                    mScoDeviceAddress = String8(device_address, MAX_DEVICE_ADDRESS_LEN);
#ifdef WITH_A2DP
                    if (mA2dpOutput != 0 &&
                        mPhoneState != AudioSystem::MODE_NORMAL) {
                        mpClientInterface->suspendOutput(mA2dpOutput);
                    }
#endif
                }
            }
            break;
        // handle output device disconnection
        case AudioSystem::DEVICE_STATE_UNAVAILABLE: {
            if (!(mAvailableOutputDevices & device)) {
                LOGW("setDeviceConnectionState() device not connected: %x", device);
                return INVALID_OPERATION;
            }


            LOGV("setDeviceConnectionState() disconnecting device %x", device);
            // remove device from available output devices
            mAvailableOutputDevices &= ~device;

#ifdef WITH_A2DP
            // handle A2DP device disconnection
            if (AudioSystem::isA2dpDevice(device)) {
                status_t status = handleA2dpDisconnection(device, device_address);
                if (status != NO_ERROR) {
                    mAvailableOutputDevices |= device;
                    return status;
                }
            } else
#endif
            {
                if (AudioSystem::isBluetoothScoDevice(device)) {
                    mScoDeviceAddress = "";
#ifdef WITH_A2DP
                    if (mA2dpOutput != 0 &&
                        mPhoneState != AudioSystem::MODE_NORMAL) {
                        mpClientInterface->restoreOutput(mA2dpOutput);
                    }
#endif
                }
            }
            } break;

        default:
            LOGE("setDeviceConnectionState() invalid state: %x", state);
            return BAD_VALUE;
        }

        // request routing change if necessary
        uint32_t newDevice = getNewDevice(mHardwareOutput, false);

        // force routing if device disconnection occurs when stream is stopped
        if ((newDevice == 0) && (state == AudioSystem::DEVICE_STATE_UNAVAILABLE))
            newDevice = getDeviceForStrategy(STRATEGY_MEDIA, false);

#ifdef WITH_A2DP
        checkOutputForAllStrategies();
        // A2DP outputs must be closed after checkOutputForAllStrategies() is executed
        if (state == AudioSystem::DEVICE_STATE_UNAVAILABLE && AudioSystem::isA2dpDevice(device)) {
            closeA2dpOutputs();
        }
#endif
        updateDeviceForStrategy();
        setOutputDevice(mHardwareOutput, newDevice);

        if (device == AudioSystem::DEVICE_OUT_WIRED_HEADSET) {
            device = AudioSystem::DEVICE_IN_WIRED_HEADSET;
        } else if (device == AudioSystem::DEVICE_OUT_BLUETOOTH_SCO ||
                   device == AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET ||
                   device == AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT) {
            device = AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        } else {
            return NO_ERROR;
        }
    }
    // handle input devices
    if (AudioSystem::isInputDevice(device)) {

        switch (state)
        {
        // handle input device connection
        case AudioSystem::DEVICE_STATE_AVAILABLE: {
            if (mAvailableInputDevices & device) {
                LOGW("setDeviceConnectionState() device already connected: %d", device);
                return INVALID_OPERATION;
            }
            mAvailableInputDevices |= device;
            }
            break;

        // handle input device disconnection
        case AudioSystem::DEVICE_STATE_UNAVAILABLE: {
            if (!(mAvailableInputDevices & device)) {
                LOGW("setDeviceConnectionState() device not connected: %d", device);
                return INVALID_OPERATION;
            }
            mAvailableInputDevices &= ~device;
            } break;

        default:
            LOGE("setDeviceConnectionState() invalid state: %x", state);
            return BAD_VALUE;
        }

        audio_io_handle_t activeInput = getActiveInput();
        if (activeInput != 0) {
            AudioInputDescriptor *inputDesc = mInputs.valueFor(activeInput);
            uint32_t newDevice = getDeviceForInputSource(inputDesc->mInputSource);
            if (newDevice != inputDesc->mDevice) {
                LOGV("setDeviceConnectionState() changing device from %x to %x for input %d",
                        inputDesc->mDevice, newDevice, activeInput);
                inputDesc->mDevice = newDevice;
                AudioParameter param = AudioParameter();
                param.addInt(String8(AudioParameter::keyRouting), (int)newDevice);
                mpClientInterface->setParameters(activeInput, param.toString());
            }
        }
#ifdef HAS_FM_RADIO
        else {
           if (device == AudioSystem::DEVICE_IN_FM_ANALOG) {
               routing_strategy strategy = getStrategy((AudioSystem::stream_type)3);
               uint32_t curOutdevice = getDeviceForStrategy(strategy);
               /* If A2DP headset is connected then route FM to Headset */
               if (curOutdevice == AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP ||
                     curOutdevice == AudioSystem::DEVICE_OUT_BLUETOOTH_SCO) {
                  curOutdevice = AudioSystem::DEVICE_OUT_WIRED_HEADSET;
               }

               if (state) {
                   // routing_strategy strategy = getStrategy((AudioSystem::stream_type)3);
                   // uint32_t curOutdevice = getDeviceForStrategy(strategy);

                   /* Get the new input descriptor for FM Rx In */
                    mfmInput = getFMInput(AUDIO_SOURCE_FM_ANALOG,8000,1,
                           AudioSystem::CHANNEL_IN_MONO,(AudioSystem::audio_in_acoustics)7);

                   /* Forcely open the current output device again for
                    * FM Rx playback path to open
                    */
                    LOGV("curOutdevice = %x",curOutdevice);
                    setOutputDevice(mHardwareOutput, curOutdevice, true);

                   /* Tell the audio flinger playback thread that
                    * FM Rx is active
                    */
                    mpClientInterface->setFMRxActive(true);
               } else {
                    int newDevice=0;
                    AudioParameter param = AudioParameter();
                    param.addInt(String8(AudioParameter::keyRouting), (int)newDevice);

                   /* Change the input device from FM to default before releasing Input */
                    mpClientInterface->setParameters(mfmInput, param.toString());
                    param.addInt(String8("fm_off"), (int)newDevice);

                   /* Close the capture handle */
                    mpClientInterface->setParameters(mfmInput, param.toString());

                   /* Release the input descriptor for FM Rx In */
                    releaseInput(mfmInput);

                   /* Close the playback handle */
                    mpClientInterface->setParameters(mHardwareOutput, param.toString());

                   /* Tell the audio flinger playback thread that
                    * FM Rx is not active now.
                    */
                    mpClientInterface->setFMRxActive(false);
                }
             }
      }
#endif
        return NO_ERROR;
    }

    LOGW("setDeviceConnectionState() invalid device: %x", device);
    return BAD_VALUE;
}

uint32_t AudioPolicyManagerALSA::getDeviceForStrategy(routing_strategy strategy, bool fromCache)
{
    uint32_t device = 0;

    if (fromCache) {
      LOGV("getDeviceForStrategy() from cache strategy %d, device %x", strategy, mDeviceForStrategy[strategy]);
      return mDeviceForStrategy[strategy];
    }

    switch (strategy) {
      case STRATEGY_DTMF:
           if (mPhoneState != AudioSystem::MODE_IN_CALL) {
           // when off call, DTMF strategy follows the same rules as MEDIA strategy
              device = getDeviceForStrategy(STRATEGY_MEDIA, false);
              break;
           }
           // when in call, DTMF and PHONE strategies follow the same rules
           // FALL THROUGH
      case STRATEGY_PHONE:
           // for phone strategy, we first consider the forced use and then the available devices by order
           // of priority
           switch (mForceUse[AudioSystem::FOR_COMMUNICATION]) {
           case AudioSystem::FORCE_BT_SCO:
              if (mPhoneState != AudioSystem::MODE_IN_CALL || strategy != STRATEGY_DTMF) {
                  device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT;
                  if (device) break;
              }
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
              if (device) break;
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO;
              if (device) break;
              // if SCO device is requested but no SCO device is available, fall back to default case
              // FALL THROUGH

           default:	// FORCE_NONE
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_WIRED_HEADPHONE;
              if (device) break;
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_WIRED_HEADSET;
              if (device) break;
#ifdef WITH_A2DP
              // when not in a phone call, phone strategy should route STREAM_VOICE_CALL to A2DP
              if (mPhoneState != AudioSystem::MODE_IN_CALL) {
                  device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP;
                  if (device) break;
                  device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;
                  if (device) break;
              }
#endif
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_EARPIECE;
              if (device == 0) {
                  LOGE("getDeviceForStrategy() earpiece device not found");
              }
              break;

        case AudioSystem::FORCE_SPEAKER:
              if (mPhoneState != AudioSystem::MODE_IN_CALL || strategy != STRATEGY_DTMF) {
                    device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT;
                    if (device) break;
              }
#ifdef WITH_A2DP
              // when not in a phone call, phone strategy should route STREAM_VOICE_CALL to
              // A2DP speaker when forcing to speaker output
              if (mPhoneState != AudioSystem::MODE_IN_CALL) {
                   device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER;
                   if (device) break;
              }
#endif
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_SPEAKER;
              if (device == 0) {
                   LOGE("getDeviceForStrategy() speaker device not found");
              }
              break;
           }
         break;

         case STRATEGY_SONIFICATION:

              // If incall, just select the STRATEGY_PHONE device: The rest of the behavior is handled by
              // handleIncallSonification().
              if (mPhoneState == AudioSystem::MODE_IN_CALL) {
                  device = getDeviceForStrategy(STRATEGY_PHONE, false);
                  break;
              }
              device = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_SPEAKER;
              if (device == 0) {
                  LOGE("getDeviceForStrategy() speaker device not found");
              }
             // The second device used for sonification is the same as the device used by media strategy
             // FALL THROUGH

        case STRATEGY_MEDIA: {
             uint32_t device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_AUX_DIGITAL;
             if (device2 == 0) {
                 device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_WIRED_HEADPHONE;
             }
             if (device2 == 0) {
                 device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_WIRED_HEADSET;
             }
#ifdef WITH_A2DP
             if (mA2dpOutput != 0) {
                if (strategy == STRATEGY_SONIFICATION && !a2dpUsedForSonification()) {
                    break;
                }
                if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP;
                }
                if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;
                }
                if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER;
                }
            }
#endif
            // Once SCO connection is connected, map strategy_media to
            // SCO headset for music streaming. BT SCO MM_UL use case
            if (mForceUse[AudioSystem::FOR_COMMUNICATION] == AudioSystem::FORCE_BT_SCO) {
                 if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT;
                 }
                 if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
                 }
                 if (device2 == 0) {
                    device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO;
                 }
           }
#ifdef HAS_FM_RADIO
            if (device2 == 0) {
                device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_FM_TRANSMIT;
            }
#endif
            if (device2 == 0) {
                device2 = mAvailableOutputDevices & AudioSystem::DEVICE_OUT_SPEAKER;
            }

            // device is DEVICE_OUT_SPEAKER if we come from case STRATEGY_SONIFICATION, 0 otherwise
            device |= device2;
            if (device == 0) {
                 LOGE("getDeviceForStrategy() speaker device not found");
            }
            } break;

         default:
            LOGW("getDeviceForStrategy() unknown strategy: %d", strategy);
              break;
         }

         LOGV("getDeviceForStrategy() strategy %d, device %x", strategy, device);
         return device;
}
#ifdef HAS_FM_RADIO
audio_io_handle_t AudioPolicyManagerALSA::getFMInput(int inputSource,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channels,
                                    AudioSystem::audio_in_acoustics acoustics)
{
    audio_io_handle_t input = 0;
    uint32_t device = 0;

    if (inputSource == AUDIO_SOURCE_FM_ANALOG)
         device = AudioSystem::DEVICE_IN_FM_ANALOG;
    else {
         /* wrong input source */
         return 0;
    }

    LOGV("getFMInput() inputSource %d, samplingRate %d, format %d, channels %x, acoustics %x", inputSource, samplingRate, format, channels, acoustics);


    AudioInputDescriptor *inputDesc = new AudioInputDescriptor();

    inputDesc->mInputSource = inputSource;
    inputDesc->mDevice = device;
    inputDesc->mSamplingRate = samplingRate;
    inputDesc->mFormat = format;
    inputDesc->mChannels = channels;
    inputDesc->mAcoustics = acoustics;
    inputDesc->mRefCount = 0;
    input = mpClientInterface->openInput(&inputDesc->mDevice,
                                    &inputDesc->mSamplingRate,
                                    &inputDesc->mFormat,
                                    &inputDesc->mChannels,
                                    inputDesc->mAcoustics);

    // only accept input with the exact requested set of parameters
    if (input == 0 ||
        (samplingRate != inputDesc->mSamplingRate) ||
        (format != inputDesc->mFormat) ||
        (channels != inputDesc->mChannels)) {
        LOGV("getInput() failed opening input: samplingRate %d, format %d, channels %d",
                samplingRate, format, channels);
        if (input != 0) {
            mpClientInterface->closeInput(input);
        }
        delete inputDesc;
        return 0;
    }
    mInputs.add(input, inputDesc);
    return input;
}
#endif

status_t AudioPolicyManagerALSA::stopOutput(audio_io_handle_t output, AudioSystem::stream_type stream)
{
        LOGV("stopOutput() output %d, stream %d", output, stream);
        ssize_t index = mOutputs.indexOfKey(output);
        if (index < 0) {
        LOGW("stopOutput() unknow output %d", output);
        return BAD_VALUE;
        }
        AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);
        routing_strategy strategy = getStrategy((AudioSystem::stream_type)stream);

        // handle special case for sonification while in call
        if (mPhoneState == AudioSystem::MODE_IN_CALL) {
        handleIncallSonification(stream, false, false);
        }
        if (outputDesc->isUsedByStrategy(strategy)) {
        // decrement usage count of this stream on the output
        outputDesc->changeRefCount(stream, -1);
        if (!outputDesc->isUsedByStrategy(strategy)) {
        // if the stream is the last of its strategy to use this output, change routing
        // in the following order or priority:
        // PHONE > SONIFICATION > MEDIA > DTMF
        uint32_t newDevice = 0;
        if (outputDesc->isUsedByStrategy(STRATEGY_PHONE)) {
            newDevice = getDeviceForStrategy(STRATEGY_PHONE);
        } else if (outputDesc->isUsedByStrategy(STRATEGY_SONIFICATION)) {
            newDevice = getDeviceForStrategy(STRATEGY_SONIFICATION);
        } else if (mPhoneState == AudioSystem::MODE_IN_CALL) {
            newDevice = getDeviceForStrategy(STRATEGY_PHONE);
        } else if (outputDesc->isUsedByStrategy(STRATEGY_MEDIA)) {
            newDevice = getDeviceForStrategy(STRATEGY_MEDIA);
        } else if (outputDesc->isUsedByStrategy(STRATEGY_DTMF)) {
            newDevice = getDeviceForStrategy(STRATEGY_DTMF);
        }
              // apply routing change if necessary.
              // insert a delay of 2 times the audio hardware latency to ensure PCM
              // buffers in audio flinger and audio hardware are emptied before the
              // routing change is executed.
              setOutputDevice(mHardwareOutput, newDevice, false, mOutputs.valueFor(mHardwareOutput)->mLatency*2);
        }
        // store time at which the last music track was stopped - see computeVolume()
	   if (stream == AudioSystem::MUSIC) {
               mMusicStopTime = systemTime();
           }
           return NO_ERROR;
           } else {
              LOGW("stopOutput() refcount is already 0 for output %d", output);
              return INVALID_OPERATION;
       }
}

// ----------------------------------------------------------------------------
// AudioPolicyManagerALSA
// ----------------------------------------------------------------------------

// ---  class factory

extern "C" AudioPolicyInterface* createAudioPolicyManager(AudioPolicyClientInterface *clientInterface)
{
    return new AudioPolicyManagerALSA(clientInterface);
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
    delete interface;
}

// Nothing currently different between the Base implementation.

AudioPolicyManagerALSA::AudioPolicyManagerALSA(AudioPolicyClientInterface *clientInterface)
    : AudioPolicyManagerBase(clientInterface)
{
}

AudioPolicyManagerALSA::~AudioPolicyManagerALSA()
{
}

}; // namespace android
