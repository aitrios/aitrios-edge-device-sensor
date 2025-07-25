/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDC_SENSOR_FW_UPDATE_LIB_LOG_H_
#define EDC_SENSOR_FW_UPDATE_LIB_LOG_H_
#include "utility_log.h"
#include "utility_log_module_id.h"

#define SENSOR_FW_UPDATE_LIB_MAJOR_EVENT_ID      (0xD200u)  // Temporary value
#define SENSOR_FW_UPDATE_LIB_EVENT_ID_MASK       (0xffffu)
#define SENSOR_FW_UPDATE_LIB_MINOR_EVENT_ID_MASK (0x00ffu)
#define SENSOR_FW_UPDATE_LIB_MODULE_ID           MODULE_ID_SYSTEM

#define SENSOR_FW_UPDATE_LIB_EVENT_ID(minor_id) \
  (SENSOR_FW_UPDATE_LIB_EVENT_ID_MASK &         \
   (SENSOR_FW_UPDATE_LIB_MAJOR_EVENT_ID |       \
    (SENSOR_FW_UPDATE_LIB_MINOR_EVENT_ID_MASK & (minor_id))))

#define SENSOR_FW_UPDATE_LIB_DLOG(level, format, ...)                    \
  UtilityLogWriteDLog(SENSOR_FW_UPDATE_LIB_MODULE_ID, level,             \
                      "%s:%d: %s " format, __FILE__, __LINE__, __func__, \
                      ##__VA_ARGS__)
#if 0
#define SENSOR_FW_UPDATE_LIB_ELOG(level, minor_id)           \
  UtilityLogWriteELog(SENSOR_FW_UPDATE_LIB_MODULE_ID, level, \
                      SENSOR_FW_UPDATE_LIB_EVENT_ID(minor_id))
#endif

#define SENSOR_FW_UPDATE_LIB_ELOG(level, minor_id) \
  printf("[" level "] ELOG: 0x%04x\n", SENSOR_FW_UPDATE_LIB_EVENT_ID(minor_id))

#define DLOG_CRITICAL(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelCritical, format, ##__VA_ARGS__)
#define DLOG_ERROR(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelError, format, ##__VA_ARGS__)
#define DLOG_WARNING(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelWarn, format, ##__VA_ARGS__)
#define DLOG_INFO(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelInfo, format, ##__VA_ARGS__)
#define DLOG_DEBUG(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelDebug, format, ##__VA_ARGS__)
#define DLOG_TRACE(format, ...) \
  SENSOR_FW_UPDATE_LIB_DLOG(kUtilityLogDlogLevelTrace, format, ##__VA_ARGS__)

#define ELOG_CRITICAL(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelCritical, minor_id);
#define ELOG_ERROR(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelError, minor_id);
#define ELOG_WARNING(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelWarn, minor_id);
#define ELOG_INFO(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelInfo, minor_id);
#define ELOG_DEBUG(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelDebug, minor_id);
#define ELOG_TRACE(minor_id) \
  SENSOR_FW_UPDATE_LIB_ELOG(kUtilityLogElogLevelTrace, minor_id);

#endif /* EDC_SENSOR_FW_UPDATE_LIB_LOG_H_ */