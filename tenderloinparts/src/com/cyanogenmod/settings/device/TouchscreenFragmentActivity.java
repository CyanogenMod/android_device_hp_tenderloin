/*
 * Copyright (C) 2011 The CyanogenMod Project
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

package com.cyanogenmod.settings.device;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;

import com.cyanogenmod.settings.device.R;

public class TouchscreenFragmentActivity extends PreferenceFragment {

    private static final String TAG = "TenderloinParts_Touchscreen";

    private static final String TOUCHSCREEN_SET_BIN = "/system/bin/ts_srv_set";
    private SharedPreferences mSharedPrefs;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!isSupported( TOUCHSCREEN_SET_BIN ))
            return;

        //Since touchscreen mode could be changed without this application we will
        //make sure there is no discrepancy between current mode of driver and shared preferences
        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(getActivity());
        syncPreferences();

        addPreferencesFromResource(R.xml.touchscreen_preferences);
        setModePrefTitle(null);

        Preference tsModePref = findPreference("touchscreen_mode_preference");
        tsModePref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                if (setTouchscreenMode((String)newValue)) {
                    setModePrefTitle((String)newValue);
                    return true;
                } else {
                    return false;
                }
            }
        });
    }

    public static boolean isSupported(String FILE) {
        return Utils.fileExists(FILE);
    }

    private void setModePrefTitle(String mode) {
        Preference tsModePref = findPreference("touchscreen_mode_preference");

        if (mode == null) {
            //Set title based on stored preference
            mode = mSharedPrefs.getString("touchscreen_mode_preference", "");
        }

        if (tsModePref != null) {
            if (mode.equalsIgnoreCase("F")) {
                tsModePref.setTitle( getString(R.string.touchscreen_mode_preference_title_finger) );
                tsModePref.setSummary( getString(R.string.touchscreen_mode_preference_summary_finger) );
            } else if (mode.equalsIgnoreCase("S")) {
                tsModePref.setTitle( getString(R.string.touchscreen_mode_preference_title_stylus) );
                tsModePref.setSummary( getString(R.string.touchscreen_mode_preference_summary_stylus) );
            }
        }
    }

    private void syncPreferences() {
        String currentMode = getCurrentTouchscreenMode();
        if (currentMode != null) {
            SharedPreferences.Editor editor = mSharedPrefs.edit();
            String [] values = getResources().getStringArray(R.array.touchscreen_mode_values);

            if (currentMode.contains("Finger")) {
                editor.putString( "touchscreen_mode_preference", values[0]);
            } else if (currentMode.contains("Stylus"))
                editor.putString( "touchscreen_mode_preference", values[1]);
            else {
                Log.e(TAG, "Unknown value returned from ts_srv_set");
            }
            editor.commit();
        }
    }

    private String getCurrentTouchscreenMode() {
        String ret = null;
        Process process;
        try {
            process = Runtime.getRuntime().exec( TOUCHSCREEN_SET_BIN + " M");
            DataInputStream is = new DataInputStream(process.getInputStream());
            process.waitFor();

            if (process.exitValue() == 0)
                ret = is.readLine();
            else
                Log.e(TAG, "Unable to get touchscreen mode");
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return ret;
    }

    private boolean setTouchscreenMode(String mode) {
        int ret = -1;
        Process process;
        try {
            process = Runtime.getRuntime().exec( TOUCHSCREEN_SET_BIN + " " + mode);
            process.waitFor();

            ret = process.exitValue();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (ret == 0)
            return true;
        else {
            Log.e(TAG, "Unable to set touchscreen mode");
            return false;
        }
    }
}
