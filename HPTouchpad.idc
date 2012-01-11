# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Input Device Calibration File for the ace touch screen.
#
# These calibration values are derived from empirical measurements
# and may not be appropriate for use with other touch screens.
# Refer to the input device calibration documentation for more details.
#

# Touch Size
#touch.touchSize.calibration = pressure

# Tool Size
# Driver reports tool size as a linear width measurement summed over
# all contact points.
#
# Raw width field measures approx. 1 unit per millimeter
# of tool size on the surface where a raw width of 1 corresponds
# to about 17mm of physical size.  Given that the display resolution
# is 10px per mm we obtain a scale factor of 10 pixels / unit and
# a bias of 160 pixels.  In addition, the raw width represents a
# sum of all contact area so we note this fact in the calibration.
#touch.toolSize.calibration = linear
#touch.toolSize.linearScale = 6.3
#touch.toolSize.linearBias = 160
#touch.toolSize.isSummed = 1

# Pressure
# Driver reports signal strength as pressure.
#
# A normal thumb touch while touching the back of the device
# typically registers about 100 signal strength units although
# this value is highly variable and is sensitive to contact area,
# manner of contact and environmental conditions.  We set the
# scale so that a normal touch with good signal strength will be
# reported as having a pressure somewhere in the vicinity of 1.0,
# a featherlight touch will be below 1.0 and a heavy or large touch
# will be above 1.0.  We don't expect these values to be accurate.
#touch.pressure.calibration = amplitude
#touch.pressure.source = default
#touch.pressure.scale = 0.01

# Size
#touch.size.calibration = normalized

# Orientation
touch.orientation.calibration = none
touch.orientationAware = 1

#
touch.deviceType = touchScreen

# make sure it's seen as an internal device and gets assigned to the internal display and not the external
device.internal = 1

