/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensor_fw_update_lib.h"

#include <pthread.h>
#include <time.h>

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "sensor_fw_update_lib_impl.h"
#include "sensor_fw_update_lib_log.h"

// Structures to save/load firmware info to/from the parameter storage
typedef struct InfoMask {
  uint8_t info_list : 1;
} InfoMask;

typedef struct InfoContainer {
  EsfParameterStorageManagerOffsetBinary info_list;
} InfoContainer;
// End

typedef enum EdcSensorFwUpdateLibState {
  kEdcSensorFwUpdateLibStateIdle,
  kEdcSensorFwUpdateLibStateWriting,
  kEdcSensorFwUpdateLibStateEraseDone,
  kEdcSensorFwUpdateLibStateError,
} EdcSensorFwUpdateLibState;

typedef struct EdcSensorFwUpdateLibContext {
  EdcSensorFwUpdateLibState state;

  EsfParameterStorageManagerItemID pstorage_id;
  InfoContainer info_container;
  bool component_info_slot_found;
  EdcSensorFwUpdateLibTarget target_component;
  EdcSensorFwUpdateLibComponentInfo component_info;
  char target_device[EDC_SENSOR_FW_UPDATE_LIB_TARGET_DEVICE_LENGTH];

  size_t total_written_size;

  EdcSensorFwUpdateLibImplHandle impl_handle;
} EdcSensorFwUpdateLibContext;

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static EdcSensorFwUpdateLibContext *s_active_context =
    EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID;

/// @brief Check if the target component is supported.
/// @param context [in] The context to check.
/// @return true if the context is valid, false otherwise.
static bool IsValidContext(const EdcSensorFwUpdateLibContext *context) {
  if (context == EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID) {
    DLOG_ERROR("Invalid context.\n");
    return false;
  }

  if (context != s_active_context) {
    DLOG_ERROR("Context is not valid.\n");
    return false;
  }

  return true;
}

/// @brief Check if the InfoMask is enabled. (for the Parameter Storage
/// Manager)
/// @param mask
/// @return true if the InfoMask is enabled, false otherwise.
static bool InfoMaskEnabled(EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(InfoMask, info_list,
                                                       mask);
}

/// @brief Get the number of slots for the firmware info by getting the size of
/// the firmware info from the parameter storage manager.
/// @param id [in] The ID of the parameter storage manager.
/// @param count [out] The number of slots for the firmware info.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult GetInfoSlotCount(
    EsfParameterStorageManagerItemID id, size_t *count) {
  if (count == NULL) {
    DLOG_ERROR("count is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  *count = 0;

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EsfParameterStorageManagerHandle pstorage_handle;
  EsfParameterStorageManagerStatus pstorage_ret =
      EsfParameterStorageManagerOpen(&pstorage_handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManagerOpen failed. (ret = %u)\n",
               pstorage_ret);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  uint32_t info_size;
  pstorage_ret =
      EsfParameterStorageManagerGetSize(pstorage_handle, id, &info_size);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManagerGetSize failed. (ret = %u)\n",
               pstorage_ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto close_pstorage_then_exit;
  }

  *count = info_size / sizeof(EdcSensorFwUpdateLibComponentInfo);
  DLOG_DEBUG("info_size = %u, count = %zu\n", info_size, *count);

close_pstorage_then_exit:
  pstorage_ret = EsfParameterStorageManagerClose(pstorage_handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManagerClose failed. (ret = %u)\n",
               pstorage_ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
  }

  return ret;
}

/// @brief Access (save or load) the firmware info.
/// @param id [in] The ID of the parameter storage manager.
/// @param data [in/out] Data to save or load.
/// @param load [in] true to load the data, false to save it.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult AccessInfo(
    EsfParameterStorageManagerItemID id, InfoContainer *data, bool load) {
  EsfParameterStorageManagerHandle pstorage_handle;
  EsfParameterStorageManagerStatus pstorage_ret =
      EsfParameterStorageManagerOpen(&pstorage_handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManagerOpen failed. (ret = %u)\n",
               pstorage_ret);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EsfParameterStorageManagerMemberInfo member_info = {
      .id      = id,
      .type    = kEsfParameterStorageManagerItemTypeOffsetBinaryPointer,
      .offset  = offsetof(InfoContainer, info_list),
      .size    = 0,  // Size is not used for offset binary pointer
      .enabled = InfoMaskEnabled,
      .custom  = NULL,
  };

  EsfParameterStorageManagerStructInfo struct_info = {
      .items_num = 1,
      .items     = &member_info,
  };

  InfoMask mask = {.info_list = 1};

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;
  if (load) {
    pstorage_ret = EsfParameterStorageManagerLoad(
        pstorage_handle, (EsfParameterStorageManagerMask)&mask,
        (EsfParameterStorageManagerData)data, &struct_info, NULL);
  } else {
    pstorage_ret = EsfParameterStorageManagerSave(
        pstorage_handle, (EsfParameterStorageManagerMask)&mask,
        (EsfParameterStorageManagerData)data, &struct_info, NULL);
  }
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManager%s failed. (ret = %u)\n",
               (load ? "Load" : "Save"), pstorage_ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
  }

  pstorage_ret = EsfParameterStorageManagerClose(pstorage_handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    DLOG_ERROR("EsfParameterStorageManagerClose failed. (ret = %u)\n",
               pstorage_ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
  }

  return ret;
}

/// @brief Save or load the firmware info from Parameter Storage Manager.
/// @param id [in] The ID of the parameter storage manager.
/// @param data [in] Data to save.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult SaveInfo(EsfParameterStorageManagerItemID id,
                                           const InfoContainer *data) {
  return AccessInfo(id, (InfoContainer *)data, false);
}

/// @brief Load the firmware info from Parameter Storage Manager.
/// @param id [in] The ID of the parameter storage manager.
/// @param data [out] Data to load.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult LoadInfo(EsfParameterStorageManagerItemID id,
                                           InfoContainer *data) {
  return AccessInfo(id, data, true);
}

/// @brief Load All component info
/// @param target_component [in] target component
/// @param target_device [in] target device
/// @param is_active [in] If true, load the list of the active components
/// otherwise, load the list of the inactive components.
/// @param info_list [in/out] The list of component info.
/// @param info_list_size [in/out] The number of slots in the info_list.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
/// @note If the *info_list is NULL, it will be allocated by this function.
static EdcSensorFwUpdateLibResult LoadAllInfo(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    bool is_active, EdcSensorFwUpdateLibComponentInfo **info_list,
    size_t *info_list_size, EsfParameterStorageManagerItemID *pstorage_id) {
  if (info_list == NULL || info_list_size == NULL) {
    DLOG_ERROR("info_list or slot_count is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EsfParameterStorageManagerItemID id;
  EdcSensorFwUpdateLibResult ret = EdcSensorFwUpdateLibImplGetPstorageItemId(
      target_component, target_device, is_active, &id);
  if (ret == kEdcSensorFwUpdateLibResultNotFound) {
    // When factory loader or firmware is used, there is no active slot and
    // EdcSensorFwUpdateLibImplGetPstorageItemId returns
    // kEdcSensorFwUpdateLibResultNotFound, in which case we return an empty
    // list.
    *info_list_size = 0;
    return kEdcSensorFwUpdateLibResultOk;
  }

  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("EdcSensorFwUpdateLibImplGetPstorageItemId failed. (ret = %u)\n",
               ret);
    return ret;
  }

  size_t slot_count;
  ret = GetInfoSlotCount(id, &slot_count);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("GetInfoSlotCount failed. (ret = %u)\n", ret);
    return ret;
  }

  EdcSensorFwUpdateLibComponentInfo *data = *info_list;

  if (slot_count > 0) {
    bool data_is_provided_by_user = (data != NULL);

    if (data_is_provided_by_user) {
      if (slot_count > *info_list_size) {
        DLOG_ERROR("The size of info_list is smaller than slot_count.\n");
        return kEdcSensorFwUpdateLibResultInvalidArgument;
      }

    } else {
      data = (EdcSensorFwUpdateLibComponentInfo *)malloc(
          (slot_count) * sizeof(EdcSensorFwUpdateLibComponentInfo));
      if (data == NULL) {
        DLOG_ERROR("Failed to allocate memory for info_data.\n");
        return kEdcSensorFwUpdateLibResultResourceExhausted;
      }
    }

    InfoContainer info_container = {
        .info_list =
            {
                .offset = 0,
                .size =
                    (slot_count) * sizeof(EdcSensorFwUpdateLibComponentInfo),
                .data = (uint8_t *)data,
            },
    };

    ret = LoadInfo(id, &info_container);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("LoadInfo failed. (ret = %u)\n", ret);
      if (!data_is_provided_by_user) {
        free(data);
      }
      return ret;
    }
  }

  *info_list      = data;
  *info_list_size = slot_count;
  if (pstorage_id != NULL) *pstorage_id = id;

  return kEdcSensorFwUpdateLibResultOk;
}

/// @brief Compare two component info structures.
/// @param info1 [in] The first component info.
/// @param info2 [in] The second component info.
/// @return true if the component info structures are equal, false otherwise.
/// @note This function compares only the hash field of the component info
/// structures.
static bool CompareComponentInfo(
    const EdcSensorFwUpdateLibComponentInfo *info1,
    const EdcSensorFwUpdateLibComponentInfo *info2) {
  if (memcmp(info1->hash, info2->hash, sizeof(info1->hash)) != 0) {
    return false;
  }
  return true;
}

/// @brief Clear the component info structure.
/// @param info [out] The component info to clear.
static void ClearComponentInfo(EdcSensorFwUpdateLibComponentInfo *info) {
  if (info == NULL) {
    DLOG_ERROR("info is NULL.\n");
    return;
  }
  memset(info, 0, sizeof(EdcSensorFwUpdateLibComponentInfo));
  info->valid = false;
}

/// @brief Set the component info container for components for which only one
/// slot is used. The info container will always set to the same slot. If the
/// same component info is found in that slot, it will return
/// kEdcSensorFwUpdateLibResultAlreadyExists.
/// @param target_component [in] The target component.
/// @param target_device [in] The target device.
/// @param context [in/out] The current context.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult SetInfoContainerSingleSlot(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    EdcSensorFwUpdateLibContext *context) {
  DLOG_INFO("Called.\n");
  if (context == NULL) {
    DLOG_ERROR("context is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EdcSensorFwUpdateLibComponentInfo data;
  EdcSensorFwUpdateLibComponentInfo *p_data = &data;
  size_t slot_count                         = 1;

  EdcSensorFwUpdateLibResult ret = LoadAllInfo(
      target_component, target_device, true, &p_data, &slot_count, NULL);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    return ret;
  }

  if (slot_count == 1) {
    if (data.valid && CompareComponentInfo(&context->component_info, &data)) {
      return kEdcSensorFwUpdateLibResultAlreadyExists;
    }
  }

  ret = EdcSensorFwUpdateLibImplGetPstorageItemId(
      target_component, target_device, false, &context->pstorage_id);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("EdcSensorFwUpdateLibImplGetPstorageItemId failed. (ret = %u)\n",
               ret);
    return ret;
  }

  context->component_info_slot_found       = false;
  context->info_container.info_list.offset = 0;
  context->info_container.info_list.size =
      sizeof(EdcSensorFwUpdateLibComponentInfo);
  context->info_container.info_list.data = (uint8_t *)&context->component_info;

  ret = kEdcSensorFwUpdateLibResultOk;

  return ret;
}

/// @brief Set the component info container for the conmponents for which more
/// than one slots are used. If the same component info is found in the
/// Parameter Storage Manager, set offset to the corresponding slot. Otherwise,
/// set offset to the first invalid slot.
/// @param target_component [in] The target component.
/// @param context [in/out] The current context.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult SetInfoContainerMultipleSlots(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    EdcSensorFwUpdateLibContext *context) {
  DLOG_INFO("Called.\n");

  EdcSensorFwUpdateLibComponentInfo *data = NULL;
  size_t slot_count                       = 0;

  EdcSensorFwUpdateLibResult ret =
      LoadAllInfo(target_component, target_device, true, &data, &slot_count,
                  &context->pstorage_id);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    return ret;
  }

  context->component_info_slot_found = false;
  size_t first_invalid_slot          = slot_count;
  if (slot_count > 0) {
    for (size_t i = 0; i < slot_count; ++i) {
      if (!data[i].valid && (first_invalid_slot == slot_count)) {
        first_invalid_slot = i;
      }

      if (data[i].valid &&
          CompareComponentInfo(&context->component_info, &data[i])) {
        context->component_info_slot_found = true;
        context->info_container.info_list.offset =
            i * sizeof(EdcSensorFwUpdateLibComponentInfo);
        memcpy(&context->component_info, &data[i],
               sizeof(EdcSensorFwUpdateLibComponentInfo));
        break;
      }
    }

    free(data);
  }

  if (!context->component_info_slot_found) {
    if (first_invalid_slot >= CONFIG_SENSOR_FW_UPDATE_LIB_MAX_AI_MODEL_COUNT) {
      DLOG_ERROR("No available slot for AI model.\n");
      return kEdcSensorFwUpdateLibResultResourceExhausted;
    }

    context->info_container.info_list.offset =
        first_invalid_slot * sizeof(EdcSensorFwUpdateLibComponentInfo);
  }

  context->info_container.info_list.size =
      sizeof(EdcSensorFwUpdateLibComponentInfo);
  context->info_container.info_list.data = (uint8_t *)&context->component_info;

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult SetInfoContainer(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    EdcSensorFwUpdateLibContext *context) {
  if (target_component == kEdcSensorFwUpdateLibTargetAIModel) {
    return SetInfoContainerMultipleSlots(target_component, target_device,
                                         context);
  }
  return SetInfoContainerSingleSlot(target_component, target_device, context);
}

/// @brief Append the component info to the list of component info to be
/// erased.
/// @param target_component [in] The target component.
/// @param target_device [in] The target device.
/// @param component_info [in] The component info to be erased.
/// @return
static EdcSensorFwUpdateLibResult RegisterForErasure(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info) {
  if (component_info == NULL) {
    DLOG_ERROR("component_info is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EdcSensorFwUpdateLibComponentInfo *data = NULL;
  size_t slot_count                       = 0;
  EsfParameterStorageManagerItemID id;
  EdcSensorFwUpdateLibResult ret = LoadAllInfo(target_component, target_device,
                                               false, &data, &slot_count, &id);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    return ret;
  }

  size_t available_slot = slot_count;
  for (size_t i = 0; i < slot_count; ++i) {
    if (!data[i].valid) {
      available_slot = i;
      break;
    }
  }

  InfoContainer info_container = {
      .info_list =
          {
              .offset =
                  available_slot * sizeof(EdcSensorFwUpdateLibComponentInfo),
              .size = sizeof(EdcSensorFwUpdateLibComponentInfo),
              .data = (uint8_t *)component_info,
          },
  };
  ret = SaveInfo(id, &info_container);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("SaveInfo failed. (ret = %u)\n", ret);
    return ret;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

/// @brief Clean up component data whose component info is registered to the
/// list of component info to be erased. If the matching component info exists
/// in the valid component info list, it will also be erased.
static void CleanUpComponentData(void) {
  const char *target_device = NULL;  // Currently not used, but kept for
                                     // future use.
  EdcSensorFwUpdateLibTarget target_component =
      kEdcSensorFwUpdateLibTargetAIModel;  // Currently only AI model is
                                           // supported for erasure.

  EdcSensorFwUpdateLibComponentInfo *info_to_be_erased = NULL;
  size_t slot_count_to_be_erased                       = 0;
  EsfParameterStorageManagerItemID id_to_be_erased;
  EdcSensorFwUpdateLibResult ret =
      LoadAllInfo(target_component, target_device, false, &info_to_be_erased,
                  &slot_count_to_be_erased, &id_to_be_erased);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    return;
  }

  if (slot_count_to_be_erased == 0) {
    DLOG_INFO("No component info slots to be erased.\n");
    return;
  }

  EdcSensorFwUpdateLibComponentInfo *info_registered = NULL;
  size_t slot_count_registered                       = 0;
  EsfParameterStorageManagerItemID id_registered;
  ret = LoadAllInfo(target_component, target_device, true, &info_registered,
                    &slot_count_registered, &id_registered);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    goto exit;
  }
  bool info_to_be_erased_updated = false;
  bool info_registered_updated   = false;

  for (size_t i = 0; i < slot_count_to_be_erased; ++i) {
    if (info_to_be_erased[i].valid) {
      ret = EdcSensorFwUpdateLibImplErase(target_component, target_device,
                                          &info_to_be_erased[i]);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        DLOG_ERROR("EdcSensorFwUpdateLibImplErase failed. (ret = %u)\n", ret);
        // If the erase operation fails, the component info is not cleared, so
        // that it will be erased in the next CleanUpComponentData call.
        continue;
      }
      // If the matching component info exists in the valid component info
      // list, it will also be erased.
      for (size_t j = 0; j < slot_count_registered; ++j) {
        if (CompareComponentInfo(&info_registered[j], &info_to_be_erased[i])) {
          ClearComponentInfo(&info_registered[j]);
          info_registered_updated = true;
        }
      }
      DLOG_INFO("Cleaned up orphaned component data at slot %zu.\n", i);
      ClearComponentInfo(&info_to_be_erased[i]);
      info_to_be_erased_updated = true;
    }
  }

  InfoContainer info_container = {};

  if (info_registered_updated) {
    info_container.info_list.offset = 0;
    info_container.info_list.size =
        slot_count_registered * sizeof(EdcSensorFwUpdateLibComponentInfo);
    info_container.info_list.data = (uint8_t *)info_registered;

    ret = SaveInfo(id_registered, &info_container);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_WARNING("SaveInfo failed. (ret = %u)\n", ret);
      goto exit;
    }
  }

  if (info_to_be_erased_updated) {
    info_container.info_list.offset = 0;
    info_container.info_list.size =
        slot_count_to_be_erased * sizeof(EdcSensorFwUpdateLibComponentInfo);
    info_container.info_list.data = (uint8_t *)info_to_be_erased;

    ret = SaveInfo(id_to_be_erased, &info_container);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_WARNING("SaveInfo failed. (ret = %u)\n", ret);
      goto exit;
    }
  }

exit:
  free(info_to_be_erased);
  free(info_registered);
}

/// @brief Get current time stamp in the ISO 8601 format.
/// @param time_stamp [out]
static void GetCurrentTimeStamp(char *time_stamp, size_t size) {
  if (time_stamp == NULL) {
    DLOG_ERROR("time_stamp is NULL.\n");
    return;
  }
  struct timespec ts;
  struct tm tm_info;

  // Get current time
  clock_gettime(CLOCK_REALTIME, &ts);
  memset(&tm_info, 0, sizeof(tm_info));
  localtime_r(&ts.tv_sec, &tm_info);

  // Get timezone offset
  int offset         = tm_info.tm_gmtoff;     // Get offset in seconds
  int offset_hours   = offset / 3600;         // Convert to hours
  int offset_minutes = (offset % 3600) / 60;  // Convert to minutes

  // Calculate fractional seconds as an integer
  int fractional_seconds =
      ts.tv_nsec / 1000000;  // Convert nanoseconds to milliseconds

  // Create ISO 8601 format string
  // Common part for both UTC and local time
  snprintf(time_stamp, size, "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
           (tm_info.tm_year + 1900) % 10000,  // Year offset from 1900
           (tm_info.tm_mon + 1) % 100,        // Month is 0-11, so +1
           tm_info.tm_mday % 100,             // Day
           tm_info.tm_hour % 100,             // Hour
           tm_info.tm_min % 100,              // Minute
           tm_info.tm_sec % 100,              // Seconds
           fractional_seconds % 1000);        // Milliseconds

  // Add timezone information
  if (offset == 0) {
    // For UTC, use 'Z'
    snprintf(time_stamp + strnlen(time_stamp, size),
             size - strnlen(time_stamp, size), "Z");
  } else {
    // For other time zones, include offset
    snprintf(time_stamp + strnlen(time_stamp, size),
             size - strnlen(time_stamp, size), "%c%02d:%02d",
             (offset >= 0) ? '+' : '-',  // Sign of timezone offset
             abs(offset_hours),          // Hours of timezone offset
             abs(offset_minutes));       // Minutes of timezone offset
  }
}

static EdcSensorFwUpdateLibResult UniquenessCheck(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info) {
  EdcSensorFwUpdateLibComponentInfo *data = NULL;
  size_t slot_count                       = 0;
  EdcSensorFwUpdateLibResult ret = LoadAllInfo(target_component, target_device,
                                               true, &data, &slot_count, NULL);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    return ret;
  }

  for (size_t i = 0; i < slot_count; ++i) {
    if (data[i].valid) {
      if (EdcSensorFwUpdateLibImplCompareComponents(
              target_component, target_device, &data[i], component_info)) {
        DLOG_ERROR(
            "Component info already exists in the Parameter Storage "
            "Manager.\n");
        ret = kEdcSensorFwUpdateLibResultAlreadyExists;
        goto exit;
      }
    }
  }

  ret = kEdcSensorFwUpdateLibResultOk;

exit:
  free(data);

  return ret;
}

// Public functions

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibBegin2(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info,
    EdcSensorFwUpdateLibHandle *handle) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  if (target_component >= kEdcSensorFwUpdateLibTargetNum) {
    DLOG_ERROR("Invalid target component: %u.\n", target_component);
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (component_info == NULL || handle == NULL) {
    DLOG_ERROR("component_info or handle is NULL.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (target_device == NULL) {
    DLOG_ERROR("target_device is NULL.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (strnlen(target_device, EDC_SENSOR_FW_UPDATE_LIB_TARGET_DEVICE_LENGTH) ==
      EDC_SENSOR_FW_UPDATE_LIB_TARGET_DEVICE_LENGTH) {
    DLOG_ERROR("target_device is too long.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (s_active_context != EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID) {
    DLOG_ERROR("Another update operation is already in progress.\n");

    ret = kEdcSensorFwUpdateLibResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  CleanUpComponentData();

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)malloc(
      sizeof(EdcSensorFwUpdateLibContext));
  if (context == NULL) {
    DLOG_ERROR("Failed to allocate memory for context.\n");
    ret = kEdcSensorFwUpdateLibResultResourceExhausted;
    goto unlock_mutex_then_exit;
  }
  context->impl_handle      = EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID;
  context->state            = kEdcSensorFwUpdateLibStateIdle;
  context->target_component = target_component;

  memcpy(&context->component_info, component_info,
         sizeof(EdcSensorFwUpdateLibComponentInfo));

  snprintf(context->target_device, sizeof(context->target_device), "%s",
           target_device);

  ret = SetInfoContainer(target_component, target_device, context);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("SetInfoContainer failed. (ret = %u)\n", ret);
    goto err_exit;
  }

  *handle          = context;
  s_active_context = context;

  ret = kEdcSensorFwUpdateLibResultOk;

  goto unlock_mutex_then_exit;

err_exit:

  free(context);
  context = NULL;

  s_active_context = EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID;
  *handle          = EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibComplete(
    EdcSensorFwUpdateLibHandle handle) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid handle.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  ret = kEdcSensorFwUpdateLibResultOk;
  if (context->state != kEdcSensorFwUpdateLibStateWriting &&
      context->state != kEdcSensorFwUpdateLibStateEraseDone) {
    DLOG_ERROR("Invalid state: %u.\n", context->state);
    ret = kEdcSensorFwUpdateLibResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (context->state == kEdcSensorFwUpdateLibStateWriting) {
    if (context->target_component == kEdcSensorFwUpdateLibTargetAIModel) {
      ret = UniquenessCheck(context->target_component, context->target_device,
                            &context->component_info);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        DLOG_ERROR("UniquenessCheck failed. (ret = %u)\n", ret);
        context->state = kEdcSensorFwUpdateLibStateError;
        goto unlock_mutex_then_exit;
      }
    }

    ret = EdcSensorFwUpdateLibImplCompleteWrite(context->impl_handle);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("EdcSensorFwUpdateLibImplCompleteWrite failed. (ret = %u)\n",
                 ret);
      context->state = kEdcSensorFwUpdateLibStateError;
      goto unlock_mutex_then_exit;
    }
  }

  if (context->state == kEdcSensorFwUpdateLibStateWriting) {
    context->component_info.valid      = true;
    context->component_info.total_size = context->total_written_size;
    // Save the component info to the parameter storage manager
    GetCurrentTimeStamp(context->component_info.update_date,
                        sizeof(context->component_info.update_date));
  } else {
    ClearComponentInfo(&context->component_info);
  }

  ret = SaveInfo(context->pstorage_id, &context->info_container);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("SaveInfo failed. (ret = %u)\n", ret);
    context->state = kEdcSensorFwUpdateLibStateError;
    goto unlock_mutex_then_exit;
  }

  context->impl_handle = EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID;

  free(context);
  context = NULL;

  s_active_context = EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID;

  ret = kEdcSensorFwUpdateLibResultOk;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCancel(
    EdcSensorFwUpdateLibHandle handle) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid handle.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (context->state == kEdcSensorFwUpdateLibStateWriting) {
    ret = EdcSensorFwUpdateLibImplCancelWrite(context->impl_handle);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_WARNING("EdcSensorFwUpdateLibImplCancelWrite failed. (ret = %u)\n",
                   ret);
      // Continue to cancel even if close fails
    }
    context->impl_handle = EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID;
  } else if (context->state == kEdcSensorFwUpdateLibStateEraseDone) {
    DLOG_WARNING("The component has already been erased. Continue anyway\n");
  }

  free(context);
  context = NULL;

  s_active_context = EDC_SENSOR_FW_UPDATE_LIB_HANDLE_INVALID;

  ret = kEdcSensorFwUpdateLibResultOk;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibWrite(
    EdcSensorFwUpdateLibHandle handle, EsfMemoryManagerHandle memory_handle,
    uint32_t size) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid handle.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }
  if (context->state != kEdcSensorFwUpdateLibStateIdle &&
      context->state != kEdcSensorFwUpdateLibStateWriting) {
    DLOG_ERROR("Invalid state: %u.\n", context->state);
    ret = kEdcSensorFwUpdateLibResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (context->state == kEdcSensorFwUpdateLibStateIdle) {
    if (context->component_info_slot_found) {
      DLOG_ERROR("The binary to be deployed already exists.\n");
      context->state = kEdcSensorFwUpdateLibStateError;
      ret            = kEdcSensorFwUpdateLibResultAlreadyExists;
      goto unlock_mutex_then_exit;
    }

    ret = EdcSensorFwUpdateLibImplBeginWrite(
        context->target_component, context->target_device,
        &context->component_info, &context->impl_handle);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("EdcSensorFwUpdateLibImplBeginWrite failed. (ret = %u)\n",
                 ret);
      goto unlock_mutex_then_exit;
    }
    context->state              = kEdcSensorFwUpdateLibStateWriting;
    context->total_written_size = 0;
  }

  ret =
      EdcSensorFwUpdateLibImplWrite(context->impl_handle, memory_handle, size);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("EdcSensorFwUpdateLibImplWrite failed. (ret = %u)\n", ret);
    context->state = kEdcSensorFwUpdateLibStateError;
    goto unlock_mutex_then_exit;
  }
  context->total_written_size += size;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibErase(
    EdcSensorFwUpdateLibHandle handle) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid handle.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }
  if (context->state != kEdcSensorFwUpdateLibStateIdle) {
    DLOG_ERROR("Invalid state: %u.\n", context->state);
    ret = kEdcSensorFwUpdateLibResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }
  if (!context->component_info_slot_found) {
    DLOG_ERROR("The binary to be erased does not exist.\n");
    context->state = kEdcSensorFwUpdateLibStateError;
    ret            = kEdcSensorFwUpdateLibResultNotFound;
    goto unlock_mutex_then_exit;
  }

  ret = RegisterForErasure(context->target_component, context->target_device,
                           &context->component_info);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("RegisterForErase failed. (ret = %u)\n", ret);
    context->state = kEdcSensorFwUpdateLibStateError;
    goto unlock_mutex_then_exit;
  }

  context->state = kEdcSensorFwUpdateLibStateEraseDone;

  CleanUpComponentData();

  ret = kEdcSensorFwUpdateLibResultOk;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibGetMaxDataSizeOnce(
    EdcSensorFwUpdateLibHandle handle, uint32_t *size) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  EdcSensorFwUpdateLibContext *context = (EdcSensorFwUpdateLibContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid handle.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  ret = EdcSensorFwUpdateLibImplGetMaxDataSizeOnce(context->impl_handle, size);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR(
        "EdcSensorFwUpdateLibImplGetMaxDataSizeOnce failed. (ret = %u)\n", ret);
    goto unlock_mutex_then_exit;
  }

  ret = kEdcSensorFwUpdateLibResultOk;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibGetComponentInfoList(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    uint32_t *list_size, EdcSensorFwUpdateLibComponentInfo *list) {
  DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to lock mutex. errno = %d\n", errno);
    return kEdcSensorFwUpdateLibResultBusy;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  if (list_size == NULL || list == NULL) {
    DLOG_ERROR("list_size or list is NULL.\n");
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  size_t slot_count = *list_size;
  ret = LoadAllInfo(target_component, target_device, true, &list, &slot_count,
                    NULL);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("LoadAllInfo failed. (ret = %u)\n", ret);
    goto unlock_mutex_then_exit;
  }

  for (size_t i = slot_count; i < *list_size; ++i) {
    ClearComponentInfo(&list[i]);
  }

  *list_size = slot_count;

  ret = kEdcSensorFwUpdateLibResultOk;

unlock_mutex_then_exit:
  if (pthread_mutex_unlock(&s_mutex) != 0) {
    DLOG_ERROR("Failed to unlock mutex. errno = %d\n", errno);
  }

  return ret;
}
