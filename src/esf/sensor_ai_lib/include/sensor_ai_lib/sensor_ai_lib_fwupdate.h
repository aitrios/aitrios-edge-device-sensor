/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_AI_LIB_FWUPDATE_H__
#define __SENSOR_AI_LIB_FWUPDATE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_manager.h"
#include "sensor_ai_lib/sensor_ai_lib_result_code.h"
#include "sensor_ai_lib/sensor_ai_lib_state.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Handle */
struct SsfSensorLibFwUpdateHandle__;
typedef struct SsfSensorLibFwUpdateHandle__ *SsfSensorLibFwUpdateHandle;

#define SSF_SENSOR_LIB_AI_MODEL_BUNDLE_LIST_MAX 4
#define SSF_SENSOR_LIB_VERSION_LENGTH           45

#define SSF_SENSOR_LIB_PARAMETER_NAME_LENGTH (32 + 1)
#define SSF_SENSOR_LIB_HASH_LENGTH           32
#define SSF_SENSOR_LIB_UPDATE_DATE_LENGTH    (32 + 1)

typedef enum SsfSensorLibFwUpdateTarget {
  kSsfSensorLibFwUpdateTargetDummy,
  kSsfSensorLibFwUpdateTargetLoader,
  kSsfSensorLibFwUpdateTargetFirmware,
  kSsfSensorLibFwUpdateTargetAIModel,
  kSsfSensorLibFwUpdateTargetNum
} SsfSensorLibFwUpdateTarget;

typedef struct SsfSensorLibComponentInfo {
  bool valid;
  char parameter_name[SSF_SENSOR_LIB_PARAMETER_NAME_LENGTH];
  char version[SSF_SENSOR_LIB_VERSION_LENGTH];
  uint8_t hash[SSF_SENSOR_LIB_HASH_LENGTH];
  int32_t total_size;
  char update_date[SSF_SENSOR_LIB_UPDATE_DATE_LENGTH];
} SsfSensorLibComponentInfo;

SsfSensorLibResult SsfSensorLibFwUpdateBegin(
    SsfSensorLibFwUpdateTarget target_component, const char *target_name,
    SsfSensorLibFwUpdateHandle *handle);

SsfSensorLibResult SsfSensorLibFwUpdateBegin2(
    SsfSensorLibFwUpdateTarget target_component, const char *target_device,
    const SsfSensorLibComponentInfo *component_info,
    SsfSensorLibFwUpdateHandle *handle);

SsfSensorLibResult SsfSensorLibFwUpdateComplete(
    SsfSensorLibFwUpdateHandle handle);

SsfSensorLibResult SsfSensorLibFwUpdateCancel(
    SsfSensorLibFwUpdateHandle handle);

SsfSensorLibResult SsfSensorLibFwUpdateWrite(
    SsfSensorLibFwUpdateHandle handle, EsfMemoryManagerHandle memory_handle,
    uint32_t size);

SsfSensorLibResult SsfSensorLibFwUpdateErase(SsfSensorLibFwUpdateHandle handle);

SsfSensorLibResult SsfSensorLibFwUpdateGetMaxDataSizeOnce(
    SsfSensorLibFwUpdateHandle handle, uint32_t *size);

SsfSensorLibResult SsfSensorLibFwUpdateGetComponentVersion(
    SsfSensorLibFwUpdateTarget target, uint32_t max_count,
    uint32_t max_version_length, char **version_list);

SsfSensorLibResult SsfSensorLibFwUpdateGetComponentInfoList(
    SsfSensorLibFwUpdateTarget target, const char *target_device,
    uint32_t *list_size, SsfSensorLibComponentInfo *list);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SENSOR_AI_LIB_FWUPDATE_H__ */
