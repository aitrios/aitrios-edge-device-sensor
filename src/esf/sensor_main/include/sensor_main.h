/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SSF_SENSOR_MAIN_H_
#define SSF_SENSOR_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  kEsfSensorOk,
  kEsfSensorFail,
} EsfSensorErrCode;

EsfSensorErrCode EsfSensorInit(void);
EsfSensorErrCode EsfSensorExit(void);
void EsfSensorPowerOFF(void);

typedef EsfSensorErrCode SsfSensorErrCode;
#define kSsfSensorOk   kEsfSensorOk
#define kSsfSensorFail kEsfSensorFail

static inline SsfSensorErrCode SsfSensorInit(void) { return EsfSensorInit(); }
static inline SsfSensorErrCode SsfSensorExit(void) { return EsfSensorExit(); }
static inline void SsfSensorPowerOFF(void) { EsfSensorPowerOFF(); }

/**
 * @brief Initialize ESF-Sensor related files
 * @return Result code.
 */
EsfSensorErrCode EsfSensorUtilitySetupFiles(void);

/**
 * @brief Verify ESF-Sensor related files
 * @return Result code.
 */
EsfSensorErrCode EsfSensorUtilityVerifyFiles(void);

/**
 * @brief Reset ESF-Sensor related files on eMMC
 * @return Result code.
 */
EsfSensorErrCode EsfSensorUtilityResetFiles(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSF_SENSOR_MAIN_H_ */
