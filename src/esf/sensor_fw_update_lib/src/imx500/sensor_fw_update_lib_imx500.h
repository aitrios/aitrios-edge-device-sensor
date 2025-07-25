/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EDC_SENSOR_FW_UPDATE_LIB_IMX500_H_
#define EDC_SENSOR_FW_UPDATE_LIB_IMX500_H_

#include "sensor_fw_update_lib.h"
typedef struct EdcSensorFwUpdateLibImx500AiModelContext
    *EdcSensorFwUpdateLibImx500AiModelHandle;
#define EDC_SENSOR_FW_UPDATE_LIB_IMX500_AI_MODEL_HANDLE_INVALID \
  ((EdcSensorFwUpdateLibImx500AiModelHandle)NULL)

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelOpen(
    const char *fpk_file_path, const char *network_info_file_path,
    char *version, size_t version_size,
    EdcSensorFwUpdateLibImx500AiModelHandle *handle);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelWrite(
    EdcSensorFwUpdateLibImx500AiModelHandle handle, const uint8_t *data,
    size_t size);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelClose(
    EdcSensorFwUpdateLibImx500AiModelHandle handle);
#endif /* EDC_SENSOR_FW_UPDATE_LIB_IMX500_H_ */
