/*
 * Copyright (C) 2013 The CyanogenMod Project
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

package org.cyanogenmod.hardware;

import org.cyanogenmod.hardware.util.FileUtils;

public class VibratorHW {

    private static String ENABLE_PATH = "/sys/devices/i2c-3/3-0033/vibrator/vib0/vib_enable";
    private static String LEVEL_PATH = "/sys/devices/i2c-3/3-0033/vibrator/vib0/vib_duty_cycle";

    public static boolean isSupported() {
        return true;
    }

    public static int getMaxIntensity()  {
        return 100;
    }
    public static int getMinIntensity()  {
        return 25;
    }
    public static int getWarningThreshold()  {
        return -1;
    }
    public static int getCurIntensity()  {
        return Integer.parseInt(FileUtils.readOneLine(LEVEL_PATH));
    }
    public static int getDefaultIntensity()  {
        return 100;
    }
    public static boolean setIntensity(int intensity)  {
        if (!FileUtils.writeLine(ENABLE_PATH, "1"))
            return false;
        if (!FileUtils.writeLine(LEVEL_PATH, String.valueOf(intensity)))
            return false;
        if (!FileUtils.writeLine(ENABLE_PATH, "0"))
            return false;
        return true;
    }
}
