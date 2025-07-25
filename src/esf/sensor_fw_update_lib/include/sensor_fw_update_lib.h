/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDC_SENSOR_FW_UPDATE_LIB_H_
#define EDC_SENSOR_FW_UPDATE_LIB_H_
#include <stdbool.h>
#include <stdint.h>

#include "memory_manager.h"

struct EdcSensorFwUpdateLibContext;
typedef struct EdcSensorFwUpdateLibContext *EdcSensorFwUpdateLibHandle;
#define EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID \
  ((EdcSensorFwUpdateLibHandle)NULL)

// Note: +1 is for the null terminator
#define EDC_SENSOR_FW_UPDATE_LIB_VERSION_LENGTH        (44 + 1)
#define EDC_SENSOR_FW_UPDATE_LIB_PARAMETER_NAME_LENGTH (32 + 1)
#define EDC_SENSOR_FW_UPDATE_LIB_HASH_LENGTH           32
#define EDC_SENSOR_FW_UPDATE_LIB_UPDATE_DATE_LENGTH    (32 + 1)
#define EDC_SENSOR_FW_UPDATE_LIB_TARGET_DEVICE_LENGTH  (32 + 1)

typedef enum {
  kEdcSensorFwUpdateLibResultOk,
  kEdcSensorFwUpdateLibResultCancelled,
  kEdcSensorFwUpdateLibResultUnknown,
  kEdcSensorFwUpdateLibResultInvalidArgument,
  kEdcSensorFwUpdateLibResultDeadlineExceeded,
  kEdcSensorFwUpdateLibResultNotFound,
  kEdcSensorFwUpdateLibResultAlreadyExists,
  kEdcSensorFwUpdateLibResultPermissionDenied,
  kEdcSensorFwUpdateLibResultResourceExhausted,
  kEdcSensorFwUpdateLibResultFailedPrecondition,
  kEdcSensorFwUpdateLibResultAborted,
  kEdcSensorFwUpdateLibResultOutOfRange,
  kEdcSensorFwUpdateLibResultUnimplemented,
  kEdcSensorFwUpdateLibResultInternal,
  kEdcSensorFwUpdateLibResultUnavailable,
  kEdcSensorFwUpdateLibResultDataLoss,
  kEdcSensorFwUpdateLibResultUnauthenticated,
  kEdcSensorFwUpdateLibResultInvalidCameraOperationParameter,
  kEdcSensorFwUpdateLibResultInvalidData,
  kEdcSensorFwUpdateLibResultBusy,
  kEdcSensorFwUpdateLibResultNum
} EdcSensorFwUpdateLibResult;

typedef enum EdcSensorFwUpdateLibTarget {
  kEdcSensorFwUpdateLibTargetLoader,
  kEdcSensorFwUpdateLibTargetFirmware,
  kEdcSensorFwUpdateLibTargetAIModel,
  kEdcSensorFwUpdateLibTargetNum
} EdcSensorFwUpdateLibTarget;

/// @brief Component information structure.
/// @note `parameter_name` is currently not used. (It is reserved for future
/// use.)
typedef struct EdcSensorFwUpdateLibComponentInfo {
  bool valid;
  char parameter_name[EDC_SENSOR_FW_UPDATE_LIB_PARAMETER_NAME_LENGTH];
  char version[EDC_SENSOR_FW_UPDATE_LIB_VERSION_LENGTH];
  uint8_t hash[EDC_SENSOR_FW_UPDATE_LIB_HASH_LENGTH];
  int32_t total_size;
  char update_date[EDC_SENSOR_FW_UPDATE_LIB_UPDATE_DATE_LENGTH];
} EdcSensorFwUpdateLibComponentInfo;

/// @brief Begin update or erase process.
/// @param target_component [in] The target component.
/// @param target_device [in] The target device. For Raspberry Pi, use "IMX500".
/// @param component_info [in] The component information. The `version` and
/// `hash` fields are required. `parameter_name` is required only when the
/// target is Calibration parameter.
/// @param handle [out] The handle to be used for subsequent operations.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibBegin2(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info,
    EdcSensorFwUpdateLibHandle *handle);

/// @brief Complete the update or erase process. After this function
/// successfully completes, the handle is invalidated.
/// @param handle [in] The handle obtained from `EdcSensorFwUpdateLibBegin2`.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibComplete(
    EdcSensorFwUpdateLibHandle handle);

/// @brief Cancel the update or erase process. Call this function when an error
/// occurs. Cannot be called after `EdcSensorFwUpdateLibErase` succeeds. After
/// this function successfully completes, the handle is invalidated.
/// @param handle [in] The handle obtained from `EdcSensorFwUpdateLibBegin2`.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCancel(
    EdcSensorFwUpdateLibHandle handle);

/// @brief Write data. This function can be called multiple times.
/// @param handle [in] The handle obtained from `EdcSensorFwUpdateLibBegin2`.
/// @param memory_handle [in] The memory manager handle in which the data is
/// stored. Only large heap memory is supported for Raspberry Pi.
/// @param size [in] The size of the data to be written.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibWrite(
    EdcSensorFwUpdateLibHandle handle, EsfMemoryManagerHandle memory_handle,
    uint32_t size);

/// @brief Erase the target component. Only the AI model can be erased.
/// @param handle [in] The handle obtained from `EdcSensorFwUpdateLibBegin2`.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibErase(
    EdcSensorFwUpdateLibHandle handle);

/// @brief Get the maximum data size that can be written at once.
/// @param handle [in] The handle obtained from `EdcSensorFwUpdateLibBegin2`.
/// @param size [out] The maximum data size that can be written at once.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibGetMaxDataSizeOnce(
    EdcSensorFwUpdateLibHandle handle, uint32_t *size);

/// @brief Get the list of component information.
/// @param target_component [in] The target component.
/// @param target_device [in] The target device.
/// @param list_size [in/out] The size of the list.
/// @param list [out] The list of component information.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibGetComponentInfoList(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    uint32_t *list_size, EdcSensorFwUpdateLibComponentInfo *list);

#endif /* EDC_SENSOR_FW_UPDATE_LIB_H_ */
