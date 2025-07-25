/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_AI_LIB_RESULT_CODE_H__
#define __SENSOR_AI_LIB_RESULT_CODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Sensor AI Library API's Result code */
typedef enum {
  kSsfSensorLibResultOk,
  kSsfSensorLibResultCancelled,
  kSsfSensorLibResultUnknown,
  kSsfSensorLibResultInvalidArgument,
  kSsfSensorLibResultDeadlineExceeded,
  kSsfSensorLibResultNotFound,
  kSsfSensorLibResultAlreadyExists,
  kSsfSensorLibResultPermissionDenied,
  kSsfSensorLibResultResourceExhausted,
  kSsfSensorLibResultFailedPrecondition,
  kSsfSensorLibResultAborted,
  kSsfSensorLibResultOutOfRange,
  kSsfSensorLibResultUnimplemented,
  kSsfSensorLibResultInternal,
  kSsfSensorLibResultUnavailable,
  kSsfSensorLibResultDataLoss,
  kSsfSensorLibResultUnauthenticated,
  kSsfSensorLibResultInvalidCameraOperationParameter,

  kSsfSensorLibResultNum
} SsfSensorLibResult;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SENSOR_AI_LIB_RESULT_CODE_H__ */
