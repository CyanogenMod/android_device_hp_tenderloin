/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device);

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_L  (3)
#define ID_GY (4)
#define ID_T  (5)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

#define LSM303DLH_ACC_DEVICE_NAME      "/dev"
#define LSM303DLH_MAG_DEVICE_NAME      "/dev"

#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Y
#define EVENT_TYPE_ACCEL_Z          ABS_Z
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL

#define EVENT_TYPE_MAGV_X           ABS_X
#define EVENT_TYPE_MAGV_Y           ABS_Y
#define EVENT_TYPE_MAGV_Z           ABS_Z

#define EVENT_TYPE_LIGHT            ABS_MISC

// 1000 LSG = 1G
#define LSG                         (1000.0f)


// conversion of acceleration data to SI units (m/s^2)
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M (1.0f/10.0f)
#define CONVERT_M_X (CONVERT_M)
#define CONVERT_M_Y (CONVERT_M)
#define CONVERT_M_Z (CONVERT_M)

// conversion of gyro data
//#define M_PI 3.14159265358979
#define CONVERT_GYRO   (M_PI / 180.0)
#define CONVERT_GYRO_X (CONVERT_GYRO)
#define CONVERT_GYRO_Y (CONVERT_GYRO)
#define CONVERT_GYRO_Z (CONVERT_GYRO)

#define RAD_P_DEG      (3.14159f/180.0f)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
