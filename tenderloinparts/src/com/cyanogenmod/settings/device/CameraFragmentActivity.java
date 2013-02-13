/*
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

package com.cyanogenmod.settings.device;

import java.io.DataInputStream;
import java.io.DataOutputStream;

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

public class CameraFragmentActivity extends PreferenceFragment implements OnPreferenceChangeListener {
    private static final String TAG = "TenderloinParts_Camera";
    private static final String CAMERA_CONF_FILE = "/data/misc/camera/config.txt";
    private SharedPreferences mSharedPrefs;

    private ListPreference mPreviewModePref;
    private ListPreference mRotationPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //Since camera mode could be changed without this application we will
        //make sure there is no discrepancy between current mode of driver and shared preferences
        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(getActivity());

        addPreferencesFromResource(R.xml.camera_preferences);

        mPreviewModePref = (ListPreference) findPreference("preview_mode_preference");
        mPreviewModePref.setOnPreferenceChangeListener(this);
        mRotationPref = (ListPreference) findPreference("rotation_mode_preference");
        mRotationPref.setOnPreferenceChangeListener(this);

        updatePreviewModeSummary(mPreviewModePref.getValue());
        updateRotationSummary(mRotationPref.getValue());
        syncPreferences(null, null);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String previewMode = (preference == mPreviewModePref) ? (String) newValue : null;
        String rotationMode = (preference == mRotationPref) ? (String) newValue : null;

        if (!syncPreferences(previewMode, rotationMode)) {
            return false;
        }

        if (preference == mPreviewModePref) {
            updatePreviewModeSummary(previewMode);
        } else if (preference == mRotationPref) {
            updateRotationSummary(rotationMode);
        }

        return true;
    }

    private void updatePreviewModeSummary(String mode) {
        int modeIndex = Integer.parseInt(mode);
        String[] summaries = getResources().getStringArray(R.array.preview_mode_summaries);

        mPreviewModePref.setSummary(summaries[modeIndex]);
    }

    private void updateRotationSummary(String angle) {
        mRotationPref.setSummary(angle + getString(R.string.rotation_unit));
    }

    private boolean syncPreferences(String previewMode, String rotationMode) {
        if (previewMode == null) {
            previewMode = mPreviewModePref.getValue();
        }
        if (rotationMode == null) {
            rotationMode = mRotationPref.getValue();
        }

        StringBuilder value = new StringBuilder();
        value.append("preview_mode=");
        value.append(previewMode);
        value.append("\n");
        value.append("rotation_mode=");
        value.append(rotationMode);
        value.append("\n");

        return Utils.writeValue(CAMERA_CONF_FILE, value.toString());
    }
}
