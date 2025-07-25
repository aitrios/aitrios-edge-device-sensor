/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_AI_LIB_SRC_SENSOR_AI_LIB_STATE_H_
#define SENSOR_AI_LIB_SRC_SENSOR_AI_LIB_STATE_H_

#include <stdbool.h>

#include "sensor_ai_lib/sensor_ai_lib_state.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct SsfSensorLibAIDevSts;

struct SsfSensorLibAIDevSts *SsfSensorLibStateGet(SsfSensorLibState *);

bool SsfSensorLibStateRelease(struct SsfSensorLibAIDevSts *);

bool SsfSensorLibStatePut(struct SsfSensorLibAIDevSts *, SsfSensorLibState);

SsfSensorLibState SsfSensorLibStatePeek(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SENSOR_AI_LIB_SRC_SENSOR_AI_LIB_STATE_H_ */
