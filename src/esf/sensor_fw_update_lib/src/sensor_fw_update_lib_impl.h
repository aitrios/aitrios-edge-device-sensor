/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDC_SENSOR_FW_UPDATE_LIB_IMPL_H_
#define EDC_SENSOR_FW_UPDATE_LIB_IMPL_H_
#include <stdbool.h>
#include <stdint.h>

#include "memory_manager.h"
#include "parameter_storage_manager.h"
#include "sensor_fw_update_lib.h"

struct EdcSensorFwUpdateLibImplContext;
typedef struct EdcSensorFwUpdateLibImplContext *EdcSensorFwUpdateLibImplHandle;

#define EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID \
  ((EdcSensorFwUpdateLibImplHandle)NULL)

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplBeginWrite(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    EdcSensorFwUpdateLibComponentInfo *component_info,
    EdcSensorFwUpdateLibImplHandle *handle);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplCompleteWrite(
    EdcSensorFwUpdateLibImplHandle handle);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplCancelWrite(
    EdcSensorFwUpdateLibImplHandle handle);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplWrite(
    EdcSensorFwUpdateLibImplHandle handle, EsfMemoryManagerHandle memory_handle,
    uint32_t size);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplErase(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplGetMaxDataSizeOnce(
    EdcSensorFwUpdateLibImplHandle handle, uint32_t *size);

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplGetPstorageItemId(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    bool is_active, EsfParameterStorageManagerItemID *item_id);

bool EdcSensorFwUpdateLibImplCompareComponents(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *info_1,
    const EdcSensorFwUpdateLibComponentInfo *info_2);
#endif /* EDC_SENSOR_FW_UPDATE_LIB_IMPL_H_ */
