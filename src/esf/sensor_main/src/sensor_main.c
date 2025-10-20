/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensor_main.h"

#include "senscord/c_api/senscord_c_api.h"
#include "sensor_main_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#define ESF_SENSOR_MAIN_LOG(level, fmt, ...)                                  \
  UtilityLogWriteDLog(MODULE_ID_SENSOR, level, "[%s:%d] " fmt "\n", __FILE__, \
                      __LINE__, ##__VA_ARGS__)

#define LOGE(fmt, ...) \
  ESF_SENSOR_MAIN_LOG(kUtilityLogDlogLevelError, fmt, ##__VA_ARGS__)

#define SENSCORD_PRINT_ERROR(target)                                     \
  do {                                                                   \
    char msg[128]   = {0};                                               \
    uint32_t length = sizeof(msg);                                       \
    senscord_get_last_error_string(SENSCORD_STATUS_PARAM_MESSAGE, msg,   \
                                   &length);                             \
    LOGE("%s err=%d: %s", target, senscord_get_last_error_cause(), msg); \
  } while (0)

extern int senscord_init_native_lib(void);
extern void senscord_deinit_native_lib(void);

static senscord_core_t core_   = 0;
static int native_initialized_ = 0;

EsfSensorErrCode EsfSensorInit(void) {
  int ret;
  senscord_config_t config;

  if (core_ != 0) {
    LOGE("senscord core has already been initialized");
    goto error;
  }

  ret = senscord_config_create(&config);
  if (ret != 0) {
    SENSCORD_PRINT_ERROR("senscord_config_create");
    goto error;
  }

  ret = senscord_core_init_with_config(&core_, config);
  if (ret != 0) {
    SENSCORD_PRINT_ERROR("senscord_core_init_with_config");
    senscord_config_destroy(config);
    goto error;
  }

  senscord_config_destroy(config);

  ret = senscord_init_native_lib();
  if (ret != 0) {
    SENSCORD_PRINT_ERROR("senscord_init_native_lib");
    goto error;
  }
  native_initialized_ = 1;

  return kEsfSensorOk;

error:
  if (core_ != 0) {
    ret = senscord_core_exit(core_);
    if (ret != 0) {
      SENSCORD_PRINT_ERROR("senscord_core_exit");
    }
    core_ = 0;
  }

  return kEsfSensorFail;
}

EsfSensorErrCode EsfSensorExit(void) {
  if (native_initialized_ != 0) {
    senscord_deinit_native_lib();
    native_initialized_ = 0;
  }

  if (core_ != 0) {
    int ret = senscord_core_exit(core_);
    if (ret != 0) {
      SENSCORD_PRINT_ERROR("senscord_core_exit");
      goto error;
    }
    core_ = 0;
  }

  return kEsfSensorOk;

error:
  return kEsfSensorFail;
}

void EsfSensorPowerOFF(void) {
  // Stub implementation
}

EsfSensorErrCode EsfSensorUtilitySetupFiles(void) { return kEsfSensorOk; }

EsfSensorErrCode EsfSensorUtilityVerifyFiles(void) {
  EsfSensorErrCode ret = EsfSensorUtilityVerifyFilesImpl();
  if (ret != kEsfSensorOk) {
    SENSCORD_PRINT_ERROR("EsfSensorUtilityVerifyFilesImpl failed");
    return ret;
  }
  return kEsfSensorOk;
}

EsfSensorErrCode EsfSensorUtilityResetFiles(void) {
  EsfSensorErrCode ret = EsfSensorUtilityResetFilesImpl();
  if (ret != kEsfSensorOk) {
    SENSCORD_PRINT_ERROR("EsfSensorUtilityResetFilesImpl failed");
    return ret;
  }
  return kEsfSensorOk;
}
