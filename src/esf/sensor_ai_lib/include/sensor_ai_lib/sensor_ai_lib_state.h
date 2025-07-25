/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_AI_LIB_STATE_H__
#define __SENSOR_AI_LIB_STATE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** State value */
typedef enum {
  kSsfSensorLibStateStandby,
  kSsfSensorLibStateReady,
  kSsfSensorLibStateRunning,
  kSsfSensorLibStateFwUpdate,
  kSsfSensorLibStateUnknown,
} SsfSensorLibState;

/**
 * @brief Get the state of lib
 * @return State code.
 */
SsfSensorLibState SsfSensorLibGetState(void);

void SsfSensorLibPowerOFF(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SENSOR_AI_LIB_STATE_H__ */
