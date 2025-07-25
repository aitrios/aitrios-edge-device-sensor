/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDC_SENSOR_FW_UPDATE_LIB_COMMON_H_
#define EDC_SENSOR_FW_UPDATE_LIB_COMMON_H_

#include <limits.h>

#include "sensor_fw_update_lib.h"

// PATH_MAX should be defined in limits.h, but provide fallback for environments
// where it's not
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    (_a < _b) ? _a : _b;    \
  })

#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    (_a > _b) ? _a : _b;    \
  })

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateDirectory(
    const char *dir_path);
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibRemoveDirectory(
    const char *dir_path);
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateEmptyFile(
    const char *file_path);

#endif /* EDC_SENSOR_FW_UPDATE_LIB_COMMON_H_ */
