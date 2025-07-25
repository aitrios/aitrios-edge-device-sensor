/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensor_fw_update_lib_impl.h"

#include <errno.h>
#include <fcntl.h>   // for open
#include <limits.h>  // For PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>  // for mkdir
#include <time.h>
#include <unistd.h>
#include <wait.h>  // for waitpid

#include "sensor_fw_update_lib_common.h"
#include "sensor_fw_update_lib_imx500.h"
#include "sensor_fw_update_lib_log.h"

#define LINE_SIZE                 (0x100)
#define TMP_DIR                   CONFIG_SENSOR_FW_UPDATE_LIB_AI_MODEL_TMP_DIRECTORY
#define TMP_NETWORK_FPK_PATH      TMP_DIR "/network.fpk"
#define TMP_NETWORK_INFO_TXT_PATH TMP_DIR "/network_info.txt"
#define TMP_FPK2RPK_LOG_PATH      TMP_DIR "/fpk2rpk.log"
#define FPK2RPK_EXECUTABLE_PATH \
  CONFIG_SENSOR_FW_UPDATE_LIB_FPK2RPK_EXECUTABLE_PATH

// wait execle execution for a maximum of EXEC_WAIT_INITIAL_INTERVAL_MS *
// (2^EXEC_WAIT_COUNT -1) seconds
#define EXEC_WAIT_INITIAL_INTERVAL_MS (100)
#define EXEC_WAIT_COUNT               (5)
#define MAX_SLEEP_MS                  (5 * 1000)

typedef enum EdcSensorFwUpdateLibImplState {
  kEdcSensorFwUpdateLibImplStateOpen,
  kEdcSensorFwUpdateLibImplStateClosed,
} EdcSensorFwUpdateLibImplState;

// Note: The order of this enum corresponds to kNetworkInfoKeys
typedef enum EdcSensorFwUpdateLibImplNetworkInfoKeyIndex {
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNetworkNum = 0,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorFormat,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K00,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K01,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K02,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K03,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K10,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K11,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K12,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K13,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K20,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K21,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K22,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K23,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH0,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH1,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH2,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH3,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH0,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH1,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH2,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH3,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_YAdd,
  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_YGain,

  kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNum,
} EdcSensorFwUpdateLibImplNetworkInfoKeyIndex;

typedef enum EdcSensorFwUpdateLibImplAiModelFormat {
  kEdcSensorFwUpdateLibImplAiModelFormatRGB = 0,
  kEdcSensorFwUpdateLibImplAiModelFormatBGR,
  kEdcSensorFwUpdateLibImplAiModelFormatY,
  kEdcSensorFwUpdateLibImplAiModelFormatBayerRGB,
  kEdcSensorFwUpdateLibImplAiModelFormatInvalid,
  kEdcSensorFwUpdateLibImplAiModelFormatDefault =
      kEdcSensorFwUpdateLibImplAiModelFormatRGB,
} EdcSensorFwUpdateLibImplAiModelFormat;

typedef struct EdcSensorFwUpdateLibImplContext EdcSensorFwUpdateLibImplContext;

typedef struct EdcSensorFwUpdateLibImplFunctions {
  EdcSensorFwUpdateLibResult (*open)(EdcSensorFwUpdateLibImplContext *context);
  EdcSensorFwUpdateLibResult (*write)(EdcSensorFwUpdateLibImplContext *context,
                                      const uint8_t *data, size_t size);
  EdcSensorFwUpdateLibResult (*close)(EdcSensorFwUpdateLibImplContext *context);
  EdcSensorFwUpdateLibResult (*erase)(
      const EdcSensorFwUpdateLibComponentInfo *component_info);
  EdcSensorFwUpdateLibResult (*complete_write)(
      EdcSensorFwUpdateLibImplContext *context);
  EdcSensorFwUpdateLibResult (*cancel_write)(
      EdcSensorFwUpdateLibImplContext *context);
} EdcSensorFwUpdateLibImplFunctions;

typedef struct EdcSensorFwUpdateLibImplContext {
  EdcSensorFwUpdateLibImplState state;

  char file_path[PATH_MAX];
  EdcSensorFwUpdateLibTarget target_component;
  const EdcSensorFwUpdateLibImplFunctions *func;

  EdcSensorFwUpdateLibComponentInfo *component_info;
  EdcSensorFwUpdateLibImx500AiModelHandle imx500_ai_model_handle;

  // For AI model update
} EdcSensorFwUpdateLibImplContext;

#define INPUT_TENSOR_MAX_CHANNELS (4)
#define NUM_ISP_OUTPUT_CHANNELS   (3)

typedef struct EdcSensorFwUpdateLibImplNetworkInfo {
  EdcSensorFwUpdateLibImplAiModelFormat format;
  long input_tensor_norm_k[NUM_ISP_OUTPUT_CHANNELS]
                          [INPUT_TENSOR_MAX_CHANNELS];  // inputTensorNorm_Kxx
  long input_norm[INPUT_TENSOR_MAX_CHANNELS];           // inputNorm_CHx
  long input_norm_shift[INPUT_TENSOR_MAX_CHANNELS];     // inputNormShift_CHx
  long input_norm_y_add;                                // inputTensorNorm_YAdd
  long input_norm_y_gain;  // input_tensorNorm_YGain
} EdcSensorFwUpdateLibImplNetworkInfo;

typedef struct EdcSensorFwUpdateLibImplAiModelInfoJson {
  const char *network_rpk_path;
  const char *network_name;
  long norm_shift[INPUT_TENSOR_MAX_CHANNELS];
  long norm_val[INPUT_TENSOR_MAX_CHANNELS];
  long div_shift;
  long div_val[INPUT_TENSOR_MAX_CHANNELS];
} EdcSensorFwUpdateLibImplAiModelInfoJson;

// List of the keys of network_info.txt
// Note: The order of this list corresponds to the enum
// EdcSensorFwUpdateLibImplNetworkInfoKeyIndex
static const char *kNetworkInfoKeys[] = {
    "networkNum",

    "inputTensorFormat",

    "inputTensorNorm_K00",  "inputTensorNorm_K01",   "inputTensorNorm_K02",
    "inputTensorNorm_K03",  "inputTensorNorm_K10",   "inputTensorNorm_K11",
    "inputTensorNorm_K12",  "inputTensorNorm_K13",   "inputTensorNorm_K20",
    "inputTensorNorm_K21",  "inputTensorNorm_K22",   "inputTensorNorm_K23",

    "inputNormShift_CH0",   "inputNormShift_CH1",    "inputNormShift_CH2",
    "inputNormShift_CH3",

    "inputNorm_CH0",        "inputNorm_CH1",         "inputNorm_CH2",
    "inputNorm_CH3",

    "inputTensorNorm_YAdd", "inputTensorNorm_YGain",
};

static bool IsSupportedTarget(EdcSensorFwUpdateLibTarget target_component) {
  switch (target_component) {
    case kEdcSensorFwUpdateLibTargetLoader:
    case kEdcSensorFwUpdateLibTargetFirmware:
      return false;
    case kEdcSensorFwUpdateLibTargetAIModel:
      break;
    default:
      return false;
  }
  return true;
}

static bool IsValidContext(const EdcSensorFwUpdateLibImplContext *context) {
  if (context == EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID) {
    DLOG_ERROR("Invalid context.\n");
    return false;
  }
  return true;
}

static void HashToHexString(size_t hash_size, const uint8_t *hash,
                            char *hex_string) {
  if (hash == NULL || hex_string == NULL) return;

  for (size_t i = 0; i < hash_size; i++) {
    snprintf(&hex_string[i * 2], 3, "%02x", hash[i]);
  }
  hex_string[hash_size * 2] = '\0';
}

static EdcSensorFwUpdateLibResult RemoveTmpDirectory(void) {
  if (access(TMP_DIR, F_OK) == 0) {
    EdcSensorFwUpdateLibResult ret =
        EdcSensorFwUpdateLibRemoveDirectory(TMP_DIR);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("Failed to remove temporary directory: %s.\n", TMP_DIR);
      return ret;
    }
  }
  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult ComponentInfoToFilePath(
    EdcSensorFwUpdateLibTarget target,
    const EdcSensorFwUpdateLibComponentInfo *info, char *file_path,
    size_t file_path_size) {
  if (info == NULL || file_path == NULL) {
    DLOG_ERROR(" info or file_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  switch (target) {
    case kEdcSensorFwUpdateLibTargetLoader:
    case kEdcSensorFwUpdateLibTargetFirmware:
      DLOG_ERROR("Unsupported target: %u.\n", target);
      return kEdcSensorFwUpdateLibResultUnimplemented;

    case kEdcSensorFwUpdateLibTargetAIModel: {
      char hash_str[EDC_SENSOR_FW_UPDATE_LIB_HASH_LENGTH * 2 + 1];
      HashToHexString(sizeof(info->hash), info->hash, hash_str);
      int r = snprintf(file_path, file_path_size,
                       CONFIG_SENSOR_FW_UPDATE_LIB_AI_MODEL_DIRECTORY
                       "/network_%.*s_%s.rpk",
                       (EDC_SENSOR_FW_UPDATE_LIB_VERSION_LENGTH - 1),
                       info->version, hash_str);
      if (r < 0 || (size_t)r >= file_path_size) {
        DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
                   file_path_size);
        return kEdcSensorFwUpdateLibResultInternal;
      }
      break;
    }

    default:
      DLOG_ERROR("Unsupported target: %u.\n", target);
      return kEdcSensorFwUpdateLibResultUnimplemented;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static void InitializeContext(EdcSensorFwUpdateLibImplContext *context) {
  if (context == NULL) return;

  context->state = kEdcSensorFwUpdateLibImplStateClosed;

  context->imx500_ai_model_handle =
      EDC_SENSOR_FW_UPDATE_LIB_IMX500_AI_MODEL_HANDLE_INVALID;
}

/// @brief Remove Newline codes (\n or \r\n) at the end of the `line`
/// @param line [in/out] string with a null terminator
/// @param line_length [in]
/// @return true if newline codes are removed.
static bool RemoveNewlineAndAtTheEnd(char *line, size_t line_size) {
  size_t line_length = strnlen(line, line_size);

  if (line[line_length - 1] == '\n') {
    --line_length;
    if (line[line_length - 1] == '\r') --line_length;
    line[line_length] = '\0';

    return true;
  }

  return false;
}

/// @brief Search for the matching key index
/// @param string [in]
/// @return The key index if found, otherwise,
/// kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNum
static EdcSensorFwUpdateLibImplNetworkInfoKeyIndex SearchForMatchingKey(
    const char *string, size_t string_size) {
  for (EdcSensorFwUpdateLibImplNetworkInfoKeyIndex key_idx = 0;
       key_idx < kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNum; ++key_idx) {
    size_t key_length = strnlen(kNetworkInfoKeys[key_idx], string_size);
    if (strncmp(kNetworkInfoKeys[key_idx], string, key_length) == 0) {
      return key_idx;
    }
  }

  return kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNum;
}

static void InitializeInputTensorInfo(
    EdcSensorFwUpdateLibImplNetworkInfo *info) {
  info->format = kEdcSensorFwUpdateLibImplAiModelFormatDefault;
  for (int j = 0; j < INPUT_TENSOR_MAX_CHANNELS; ++j) {
    for (int i = 0; i < NUM_ISP_OUTPUT_CHANNELS; ++i)
      info->input_tensor_norm_k[i][j] = 0;
    info->input_norm[j]       = 0;
    info->input_norm_shift[j] = 0;
  }
  info->input_norm_y_add  = 0;
  info->input_norm_y_gain = 0;
};

static EdcSensorFwUpdateLibImplAiModelFormat Str2FormatEnum(
    const char *string, size_t string_size) {
  if (0 == strncmp(string, "RGB", string_size)) {
    return kEdcSensorFwUpdateLibImplAiModelFormatRGB;
  } else if (0 == strncmp(string, "BGR", string_size)) {
    return kEdcSensorFwUpdateLibImplAiModelFormatBGR;
  } else if (0 == strncmp(string, "Y", string_size)) {
    return kEdcSensorFwUpdateLibImplAiModelFormatY;
  } else if (0 == strncmp(string, "BayerRGB", string_size)) {
    return kEdcSensorFwUpdateLibImplAiModelFormatBayerRGB;
  }
  DLOG_ERROR("Invalid format: %s\n", string);

  return kEdcSensorFwUpdateLibImplAiModelFormatInvalid;
}

/// @brief strchr with size limit of `str`
/// @param str [in]
/// @param c [in]
/// @param n [in]
/// @return
static char *strnchr(const char *str, char c, size_t n) {
  const char *end = str + n;
  while (str < end) {
    if (*str == c) return (char *)str;
    ++str;
  }
  return NULL;
}

/// @brief Get the pointer to the beginning of the value
/// @param line [in] The string with format of "KEY=VALUE"
/// @param line_size [in] Size of `line`
/// @param value_length [out] length of "VALUE"
/// @return the pointer to the beginning of the "VALUE" if format is OK,
/// otherwise, NULL.
static const char *GetValuePointer(const char *line, size_t line_size,
                                   size_t *value_length) {
  size_t line_length = strnlen(line, line_size);

  const char *value_pointer = strnchr(line, '=', line_size);
  if (value_pointer == NULL) {
    DLOG_ERROR("Invalid format: %*.s\n", (int)line_length, line);
    return NULL;
  }

  if (++value_pointer >= line + line_size) {  // ++ is to skip '='
    // In case `line[line_size-1] == '='`
    DLOG_ERROR("Invalid format: %*.s\n", (int)line_length, line);
    return NULL;
  }

  if (value_length != NULL) {
    size_t value_size = line + line_size - value_pointer;
    *value_length     = strnlen(value_pointer, value_size);
  }

  return value_pointer;
}

/// @brief Parse `value` to the corresponding `info` member.
/// @param key_idx [in]
/// @param value [in] MUST end with a null terminator (because strtol()
/// requires that)
/// @param value_size [out]
/// @param info [in/out]
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult ParseAiModelInfo(
    EdcSensorFwUpdateLibImplNetworkInfoKeyIndex key_idx, const char *value,
    size_t value_size, EdcSensorFwUpdateLibImplNetworkInfo *info) {
  if (*value == '\0') {
    DLOG_ERROR("value is \"\"\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (key_idx ==
      kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorFormat) {
    info->format = Str2FormatEnum(value, value_size);
    return kEdcSensorFwUpdateLibResultOk;
  }

  // Convert form string to long
  char *end_pointer = NULL;
  errno             = 0;
  long parsed       = strtol(value, &end_pointer, 0);
  if (errno != 0 || *end_pointer != '\0') {
    DLOG_ERROR("strtol failed. (errno = %d, value = \"%s\")\n", errno, value);
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  switch (key_idx) {
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNetworkNum:
      if (parsed != 1) {
        DLOG_CRITICAL(
            "Invalid NetworkNum = %ld. (Only NetworkNum = 1 is supported)\n",
            parsed);
        return kEdcSensorFwUpdateLibResultInvalidData;
      }
      break;

    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K00:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K01:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K02:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K03:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K10:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K11:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K12:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K13:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K20:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K21:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K22:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K23: {
      size_t ij =
          key_idx -
          kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputTensorNorm_K00;
      size_t i                        = ij / INPUT_TENSOR_MAX_CHANNELS;
      size_t j                        = ij % INPUT_TENSOR_MAX_CHANNELS;
      info->input_tensor_norm_k[i][j] = parsed;
      break;
    }

    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH0:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH1:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH2:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH3: {
      size_t i = key_idx -
                 kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNormShift_CH0;
      info->input_norm_shift[i] = parsed;
      break;
    }

    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH0:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH1:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH2:
    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH3: {
      size_t i =
          key_idx - kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_CH0;
      info->input_norm[i] = parsed;
      break;
    }

    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_YAdd:
      info->input_norm_y_add = parsed;
      break;

    case kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexInputNorm_YGain:
      info->input_norm_y_gain = parsed;
      break;

    default:
      return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult SetAiModelInfoJson(
    const EdcSensorFwUpdateLibImplNetworkInfo *network_info,
    EdcSensorFwUpdateLibImplAiModelInfoJson *json) {
  if (network_info == NULL || json == NULL) {
    DLOG_ERROR("network_info or json is NULL\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  size_t num_channels = 0;
  switch (network_info->format) {
    case kEdcSensorFwUpdateLibImplAiModelFormatRGB: {
      num_channels = 3;
      for (size_t i = 0; i < num_channels; ++i) {
        json->norm_val[i]   = network_info->input_tensor_norm_k[i][3];
        json->norm_shift[i] = 4;  // fixed
        json->div_val[i]    = network_info->input_tensor_norm_k[i][i];
      }
      json->div_shift = 6;  // fixed;
      break;
    }

    case kEdcSensorFwUpdateLibImplAiModelFormatBGR: {
      num_channels = 3;
      for (size_t i = 0; i < num_channels; ++i) {
        json->norm_val[i]   = network_info->input_tensor_norm_k[i][3];
        json->norm_shift[i] = 4;  // fixed
        json->div_val[i]    = network_info->input_tensor_norm_k[i][2 - i];
      }
      json->div_shift = 6;  // fixed;
      break;
    }

    case kEdcSensorFwUpdateLibImplAiModelFormatY: {
      num_channels        = 1;
      json->norm_val[0]   = network_info->input_norm_y_add;
      json->norm_shift[0] = 0;  // fixed
      json->div_val[0]    = network_info->input_norm_y_gain;
      json->div_shift     = 5;  // fixed
      break;
    }

    case kEdcSensorFwUpdateLibImplAiModelFormatBayerRGB: {
      num_channels = 4;
      for (size_t i = 0; i < num_channels; ++i) {
        json->norm_val[i]   = network_info->input_norm[i];
        json->norm_shift[i] = network_info->input_norm_shift[i];
        json->div_val[i]    = network_info->input_norm_y_gain;
      }
      json->div_shift = 5;  // fixed
      break;
    }

    case kEdcSensorFwUpdateLibImplAiModelFormatInvalid:
    default:
      DLOG_ERROR("Invalid format.\n");
      return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  // Values not used
  for (size_t i = num_channels; i < INPUT_TENSOR_MAX_CHANNELS; ++i) {
    json->norm_val[i]   = 0;
    json->norm_shift[i] = 0;
    json->div_val[i]    = 1;
  }

  for (size_t i = 0; i < ARRAY_SIZE(json->div_val); ++i) {
    if (json->div_val[i] == 0) {
      DLOG_CRITICAL("div_val[%zu] = 0 in the JSON file for AI model info.\n",
                    i);
      return kEdcSensorFwUpdateLibResultInvalidData;
    }
  }

  return kEdcSensorFwUpdateLibResultOk;
}

#define INDENT "    "

static void fprintf_long_array(FILE *fp, const long *array, size_t array_size) {
  if (fp == NULL || array == NULL) {
    return;
  }

  fprintf(fp, "[");
  for (size_t i = 0; i < array_size; ++i) {
    if (i != 0) fprintf(fp, ", ");
    fprintf(fp, "%ld", array[i]);
  }
  fprintf(fp, "]");
}

static EdcSensorFwUpdateLibResult SaveAiModelInfoAsJsonFile(
    const char *file_path,
    const EdcSensorFwUpdateLibImplAiModelInfoJson *info) {
  FILE *fp = fopen(file_path, "w");
  if (fp == NULL) {
    DLOG_ERROR("Failed to open %s (errno = %d\n", file_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  fprintf(fp, "{\n");
  fprintf(fp, INDENT "\"%.255s\": {\n", info->network_name);
  fprintf(fp, INDENT INDENT "\"network_file\": \"%.255s\",\n",
          info->network_rpk_path);
  fprintf(fp, INDENT INDENT "\"save_input_tensor\": {\n");
  fprintf(fp, INDENT INDENT INDENT
          "\"filename\": \"/home/pi/input_tensor.raw\",\n");   // Fixed value
  fprintf(fp, INDENT INDENT INDENT "\"num_tensors\": 10,\n");  // Fixed value
  fprintf(fp, INDENT INDENT INDENT "\"norm_val\": ");
  fprintf_long_array(fp, info->norm_val, ARRAY_SIZE(info->norm_val));
  fprintf(fp, ",\n");
  fprintf(fp, INDENT INDENT INDENT "\"norm_shift\": ");
  fprintf_long_array(fp, info->norm_shift, ARRAY_SIZE(info->norm_shift));
  fprintf(fp, ",\n");
  fprintf(fp, INDENT INDENT INDENT "\"div_val\": ");
  fprintf_long_array(fp, info->div_val, ARRAY_SIZE(info->div_val));
  fprintf(fp, ",\n");
  fprintf(fp, INDENT INDENT INDENT "\"div_shift\": %ld\n", info->div_shift);
  fprintf(fp, INDENT INDENT "}\n");
  fprintf(fp, INDENT "}\n");
  fprintf(fp, "}\n");

  if (fclose(fp) != 0) {
    DLOG_ERROR("Failed to close file: %s (errno = %d)\n", file_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }
  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult CreateJsonFileForAiModel(
    const char *network_name, const char *network_rpk_path,
    const char *json_file_path) {
  FILE *fp = fopen(TMP_NETWORK_INFO_TXT_PATH, "r");
  if (fp == NULL) {
    DLOG_ERROR("Failed to open file: " TMP_NETWORK_INFO_TXT_PATH
               " (errno = %d)\n",
               errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  char line[LINE_SIZE];

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;
  EdcSensorFwUpdateLibImplNetworkInfo network_info;
  InitializeInputTensorInfo(&network_info);

  while (fgets(line, sizeof(line), fp) != NULL) {
    // fgets guarantees that the string in 'line` ends with a null terminator.

    if (!RemoveNewlineAndAtTheEnd(line, sizeof(line))) {
      DLOG_ERROR("The line is longer than the buffer size (%zu)", sizeof(line));
      ret = kEdcSensorFwUpdateLibResultInternal;
      goto close;
    }

    EdcSensorFwUpdateLibImplNetworkInfoKeyIndex key_idx =
        SearchForMatchingKey(line, sizeof(line));
    if (key_idx >= kEdcSensorFwUpdateLibImplNetworkInfoKeyIndexNum) {
      // No matching key, which means infomation in this line will not be used
      // to create the JSON file.
      continue;
    }

    size_t value_length = 0;
    const char *value   = GetValuePointer(line, sizeof(line), &value_length);
    if (value == NULL) {
      DLOG_WARNING("Invalid format: %s\n", line);
      continue;
    }

    // Here, `value` must be (a pointer to) a null-terminated string. It is
    // guaranteed as `value` is a part of `line`, which is a null-terminated
    // string.
    ret = ParseAiModelInfo(key_idx, value, value_length, &network_info);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("ParseAiModelInfo failed. ret = %u\n", ret);
      goto close;
    }

  }  // while (fgets(line, sizeof(line), fp) != NULL)

  EdcSensorFwUpdateLibImplAiModelInfoJson ai_model_info_json;
  ret = SetAiModelInfoJson(&network_info, &ai_model_info_json);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("SetAiModelInfoJson failed. ret = %u\n", ret);
    goto close;
  }
  ai_model_info_json.network_name     = network_name;
  ai_model_info_json.network_rpk_path = network_rpk_path;

  ret = SaveAiModelInfoAsJsonFile(json_file_path, &ai_model_info_json);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("SaveAiModelInfoAsJsonFile failed. ret = %u\n", ret);
    goto close;
  }

close:
  if (fclose(fp) != 0) {
    DLOG_ERROR("Failed to close file: " TMP_NETWORK_INFO_TXT_PATH
               " (errno = %d)\n",
               errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
  }
  return ret;
}

static EdcSensorFwUpdateLibResult OpenAiModel(
    EdcSensorFwUpdateLibImplContext *context) {
  if (context == NULL) {
    DLOG_ERROR("context is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  // Create the tmp directory if it does not exist.
  if (mkdir(TMP_DIR, 0755) != 0 && errno != EEXIST) {
    DLOG_ERROR("Failed to create directory: %s (errno = %d).\n", TMP_DIR,
               errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  return EdcSensorFwUpdateLibImx500AiModelOpen(
      TMP_NETWORK_FPK_PATH, TMP_NETWORK_INFO_TXT_PATH,
      context->component_info->version,
      sizeof(context->component_info->version),
      &context->imx500_ai_model_handle);
}

static EdcSensorFwUpdateLibResult WriteAiModel(
    EdcSensorFwUpdateLibImplContext *context, const uint8_t *data,
    size_t size) {
  if (context == NULL) {
    DLOG_ERROR("context is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  return EdcSensorFwUpdateLibImx500AiModelWrite(context->imx500_ai_model_handle,
                                                data, size);
}

static EdcSensorFwUpdateLibResult CloseAiModel(
    EdcSensorFwUpdateLibImplContext *context) {
  if (context == NULL) {
    DLOG_ERROR("context is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (context->imx500_ai_model_handle !=
      EDC_SENSOR_FW_UPDATE_LIB_IMX500_AI_MODEL_HANDLE_INVALID) {
    EdcSensorFwUpdateLibResult ret =
        EdcSensorFwUpdateLibImx500AiModelClose(context->imx500_ai_model_handle);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("Failed to close AI model handle. ret = %u\n", ret);
      return ret;
    }
    context->imx500_ai_model_handle =
        EDC_SENSOR_FW_UPDATE_LIB_IMX500_AI_MODEL_HANDLE_INVALID;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

/// @brief Read a log file and output the content as DLOG (level = info)
/// @param file_path
static void File2DlogInfo(const char *file_path) {
  if (file_path == NULL) {
    DLOG_ERROR("file_path is NULL.\n");
    return;
  }

  FILE *fp = fopen(file_path, "r");
  if (fp == NULL) {
    DLOG_ERROR("Failed to open file: %s (errno = %d).\n", file_path, errno);
    return;
  }

  char line[LINE_SIZE];
  while (fgets(line, sizeof(line), fp) != NULL) {
    DLOG_INFO("%s: %s", file_path, line);
  }
  if (fclose(fp) != 0) {
    DLOG_ERROR("Failed to close file: %s (errno = %d).\n", file_path, errno);
  }
}

static void SleepMs(long ms) {
  ms = MIN(ms, MAX_SLEEP_MS);
  ms = MAX(ms, 0);

  struct timespec req = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000};
  struct timespec rem;
  while (nanosleep(&req, &rem) == -1) {
    if (errno == EINTR) {
      req.tv_sec  = rem.tv_sec;
      req.tv_nsec = rem.tv_nsec;
    } else {
      DLOG_ERROR("nanosleep failed. errno = %d\n", errno);
      break;
    }
  }
}

/// @brief Execute fpk2rpk command.
/// @param rpk_path [in] The path to the output RPK file.
/// @note This function will never return.
static void ExecuteFpkToRpk(const char *rpk_path) {
  // Redirect stdout and stderr to a log file
  int fd = open(TMP_FPK2RPK_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    DLOG_ERROR("Failed to open log file: %s (errno = %d)\n",
               TMP_FPK2RPK_LOG_PATH, errno);
    exit(1);
  }
  if ((dup2(fd, STDOUT_FILENO) == -1) || (dup2(fd, STDERR_FILENO) == -1)) {
    DLOG_ERROR("dup2 failed. errno = %d\n", errno);
    close(fd);
    exit(1);
  }
  close(fd);

  const char *envp[] = {NULL};
  execle(FPK2RPK_EXECUTABLE_PATH, FPK2RPK_EXECUTABLE_PATH, "-r",
         TMP_NETWORK_INFO_TXT_PATH, "-o", rpk_path, TMP_NETWORK_FPK_PATH, NULL,
         envp);
  DLOG_ERROR("fpk2rpk command failed. errno = %d\n", errno);
  exit(1);
}

/// @brief Wait for the child process to finish.
/// If the child process does not finish within a certain time,
/// it will kill the child process and return an error.
/// @param pid [in] The process ID of the child process.
/// @return kEdcSensorFwUpdateLibResultOk on success,
/// or an error code on failure.
static EdcSensorFwUpdateLibResult WaitForChildProcess(pid_t pid) {
  int status;
  bool is_timeout = true;
  long timeout_ms = EXEC_WAIT_INITIAL_INTERVAL_MS;
  for (int i = 0; i < EXEC_WAIT_COUNT; ++i) {
    SleepMs(timeout_ms);

    int r = waitpid(pid, &status, WNOHANG);
    if (r == 0) {
      // The child process is still running
      timeout_ms *= 2;
      continue;

    } else if (r == -1) {
      DLOG_ERROR("waitpid failed. errno = %d\n", errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }

    // The child process has finished
    is_timeout = false;
    break;
  }  // for (int i = 0; i < EXEC_WAIT_COUNT; ++i

  if (is_timeout) {
    DLOG_ERROR("fpk2rpk command timed out.\n");
    if (kill(pid, SIGKILL) != 0) {
      DLOG_ERROR("kill failed. errno = %d\n", errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }

    // Wait 100 ms for the child process to finish after killing it
    SleepMs(100);

    if (waitpid(pid, &status, WNOHANG) == -1) {
      DLOG_ERROR("waitpid failed after kill. errno = %d\n", errno);
    }
    return kEdcSensorFwUpdateLibResultInternal;
  }

  if (WIFEXITED(status) != 0) {
    if (WEXITSTATUS(status) != 0) {
      DLOG_ERROR("fpk2rpk command failed. ret = %d\n", WEXITSTATUS(status));
      return kEdcSensorFwUpdateLibResultInternal;
    }
  } else {
    DLOG_ERROR("fpk2rpk command exitted abnormally.\n");
    return kEdcSensorFwUpdateLibResultInternal;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

/// @brief Execute fpk2rpk command in a child process.
/// @param rpk_path
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
static EdcSensorFwUpdateLibResult ConvertFpkToRpk(const char *rpk_path) {
  pid_t pid = fork();

  if (pid < 0) {
    DLOG_ERROR("fork failed. (errno = %d)\n", errno);
    return kEdcSensorFwUpdateLibResultInternal;

  } else if (pid == 0) {
    ExecuteFpkToRpk(rpk_path);

    // This line should never be reached, but in case it is reached,
    // log an error and exit with a non-zero status.
    DLOG_CRITICAL("ExecuteFpkToRpk() returned unexpectedly.\n");
    exit(1);

  } else {
    EdcSensorFwUpdateLibResult ret = WaitForChildProcess(pid);
    File2DlogInfo(TMP_FPK2RPK_LOG_PATH);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("WaitForChildProcess failed. (ret = %u)\n", ret);
      return ret;
    }
  }

  return kEdcSensorFwUpdateLibResultOk;
}

#define AI_MODEL_BUNDLE_ID_SIZE   (6)
#define AI_MODEL_BUNDLE_ID_OFFSET (6)

static EdcSensorFwUpdateLibResult ComponentInfoToJsonFilePath(
    const EdcSensorFwUpdateLibComponentInfo *component_info, char *file_path,
    size_t file_path_size) {
  // +1 is for null terminator
  char ai_model_bundle_id[AI_MODEL_BUNDLE_ID_SIZE + 1];

  // The format of version is “YYYYYYXXXXXXAABB”
  // Here, "XXXXXX" is the AI model bundle ID.
  memcpy(ai_model_bundle_id,
         &component_info->version[AI_MODEL_BUNDLE_ID_OFFSET],
         AI_MODEL_BUNDLE_ID_SIZE);
  ai_model_bundle_id[AI_MODEL_BUNDLE_ID_SIZE] = '\0';

  int r = snprintf(file_path, file_path_size,
                   CONFIG_SENSOR_FW_UPDATE_LIB_AI_MODEL_JSON_DIRECTORY
                   "/custom_%6s.json",
                   ai_model_bundle_id);
  if (r < 0 || (size_t)r >= file_path_size) {
    DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
               file_path_size);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult CompleteWriteAiModel(
    EdcSensorFwUpdateLibImplContext *context) {
  if (context == NULL) {
    DLOG_ERROR("context is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  // Convert network.fpk and network_info.txt to rpk format.
  EdcSensorFwUpdateLibResult ret = ComponentInfoToFilePath(
      context->target_component, context->component_info, context->file_path,
      sizeof(context->file_path));
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToFilePath failed. (ret = %u)\n", ret);
    return ret;
  }

  ret = ConvertFpkToRpk(context->file_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ConvertFpkToRpk failed. (ret = %u)\n", ret);
    return ret;
  }

  char json_file_path[PATH_MAX];
  ret = ComponentInfoToJsonFilePath(context->component_info, json_file_path,
                                    sizeof(json_file_path));
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToJsonFilePath failed. (ret = %u)\n", ret);
    return ret;
  }

  char network_name[PATH_MAX];
  int r = snprintf(network_name, sizeof(network_name), "imx500_no_process");
  if (r < 0 || (size_t)r >= sizeof(network_name)) {
    DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
               sizeof(network_name));
    return kEdcSensorFwUpdateLibResultInternal;
  }

  ret = CreateJsonFileForAiModel(network_name, context->file_path,
                                 json_file_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("CreateJsonFileForAiModel failed. (ret = %u)\n", ret);
    return ret;
  }

// If this is defined, rpk and json files will be saved with the old file names
// AND new file names. (i.e., there will be two copies of the same files.)
// This is temporal implementation to keep backward compatibility.
// TODO: Remove this after the old file names are no longer used.
#define SAVE_WITH_OLD_NAME
#ifdef SAVE_WITH_OLD_NAME
#define OLD_RPK_PATH \
  CONFIG_SENSOR_FW_UPDATE_LIB_AI_MODEL_DIRECTORY "/network.rpk"
#define OLD_JSON_PATH \
  CONFIG_SENSOR_FW_UPDATE_LIB_AI_MODEL_JSON_DIRECTORY "/custom.json"

  ret = ConvertFpkToRpk(OLD_RPK_PATH);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ConvertFpkToRpk failed for old name. (ret = %u)\n", ret);
    return ret;
  }
  ret = CreateJsonFileForAiModel(network_name, OLD_RPK_PATH, OLD_JSON_PATH);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("CreateJsonFileForAiModel failed for old name. (ret = %u)\n",
               ret);
    return ret;
  }
#endif /* SAVE_WITH_OLD_NAME */

  EdcSensorFwUpdateLibResult tmp_ret = RemoveTmpDirectory();
  if (tmp_ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_WARNING("RemoveTmpDirectory failed. (ret = %u) Continue anyway.\n",
                 tmp_ret);
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult CancelWriteAiModel(
    EdcSensorFwUpdateLibImplContext *context) {
  char file_path[PATH_MAX];

  EdcSensorFwUpdateLibResult ret = ComponentInfoToFilePath(
      context->target_component, context->component_info, file_path,
      sizeof(file_path));
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToFilePath failed. (ret = %u)\n", ret);
    return ret;
  }
  if (access(file_path, F_OK) == 0) {
    if (remove(file_path) != 0) {
      DLOG_WARNING("Failed to delete file: %s (errno = %d)\n", file_path,
                   errno);
    }
  } else {
    DLOG_INFO("File does not exist: %s\n", file_path);
  }

  ret = ComponentInfoToJsonFilePath(context->component_info, file_path,
                                    sizeof(file_path));
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToJsonFilePath failed. (ret = %u)\n", ret);
    return ret;
  }
  if (access(file_path, F_OK) == 0) {
    if (remove(file_path) != 0) {
      DLOG_WARNING("Failed to delete file: %s (errno = %d)\n", file_path,
                   errno);
    }
  } else {
    DLOG_INFO("File does not exist: %s\n", file_path);
  }

  EdcSensorFwUpdateLibResult tmp_ret = RemoveTmpDirectory();
  if (tmp_ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_WARNING("RemoveTmpDirectory failed. (ret = %u) Continue anyway.\n",
                 tmp_ret);
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult EraseAiModel(
    const EdcSensorFwUpdateLibComponentInfo *component_info) {
  if (component_info == NULL) {
    DLOG_ERROR("component_info is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  bool error_occurred = false;

  char file_path[PATH_MAX];
  EdcSensorFwUpdateLibResult ret =
      ComponentInfoToFilePath(kEdcSensorFwUpdateLibTargetAIModel,
                              component_info, file_path, sizeof(file_path));

  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToFilePath failed. (ret = %u)\n", ret);
    error_occurred = true;

  } else if (access(file_path, F_OK) == 0) {
    if (remove(file_path) != 0) {
      DLOG_ERROR("Failed to delete file: %s (errno = %d)\n", file_path, errno);
    }

  } else {
    DLOG_INFO("File does not exist: %s\n", file_path);
  }

  // Remove the json file.
  ret =
      ComponentInfoToJsonFilePath(component_info, file_path, sizeof(file_path));
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToFilePath failed. (ret = %u)\n", ret);
    error_occurred = true;
  } else if (access(file_path, F_OK) == 0) {
    if (remove(file_path) != 0) {
      DLOG_ERROR("Failed to delete file: %s (errno = %d)\n", file_path, errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }

  } else {
    DLOG_ERROR("ComponentInfoToJsonFilePath failed. (ret = %u)\n", ret);
  }

  if (error_occurred) {
    return kEdcSensorFwUpdateLibResultInternal;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static const EdcSensorFwUpdateLibImplFunctions
    kEdcSensorFwUpdateLibAiModelFunctions = {
        .open           = OpenAiModel,
        .write          = WriteAiModel,
        .close          = CloseAiModel,
        .erase          = EraseAiModel,
        .complete_write = CompleteWriteAiModel,
        .cancel_write   = CancelWriteAiModel,
};

static const EdcSensorFwUpdateLibImplFunctions *GetFunctions(
    EdcSensorFwUpdateLibTarget target_component) {
  switch (target_component) {
    case kEdcSensorFwUpdateLibTargetAIModel:
      return &kEdcSensorFwUpdateLibAiModelFunctions;

    default:
      DLOG_ERROR("Unsupported target component: %u.\n", target_component);
      return NULL;
  }
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplBeginWrite(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    EdcSensorFwUpdateLibComponentInfo *component_info,
    EdcSensorFwUpdateLibImplHandle *handle) {
  (void)target_device;  // Unused

  if (!IsSupportedTarget(target_component)) {
    DLOG_ERROR("Unsupported target component: %u.\n", target_component);
    return kEdcSensorFwUpdateLibResultUnimplemented;
  }

  if (component_info == NULL || handle == NULL) {
    DLOG_ERROR("component_info or handle is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  *handle = EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID;

  EdcSensorFwUpdateLibImplContext *context =
      (EdcSensorFwUpdateLibImplContext *)malloc(
          sizeof(EdcSensorFwUpdateLibImplContext));
  if (context == NULL) {
    DLOG_ERROR("Failed to allocate memory for context.\n");
    return kEdcSensorFwUpdateLibResultResourceExhausted;
  }

  context->target_component = target_component;
  context->component_info   = component_info;
  InitializeContext(context);

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  context->func = GetFunctions(target_component);
  if (context->func == NULL) {
    DLOG_ERROR("No functions registered for target component: %u.\n",
               target_component);
    ret = kEdcSensorFwUpdateLibResultUnimplemented;
    goto err_exit;
  }

  ret = ComponentInfoToFilePath(target_component, component_info,
                                context->file_path, sizeof(context->file_path));

  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("ComponentInfoToFilePath failed. (ret = %u)\n", ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto err_exit;
  }

  if (context->func->open == NULL) {
    DLOG_ERROR("func->open is not registered for target component: %u.\n",
               context->target_component);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto err_exit;
  }
  context->state = kEdcSensorFwUpdateLibImplStateOpen;

  ret = context->func->open(context);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("func->open failed: %u\n", ret);
    goto err_exit;
  }

  // Note: If any error occurs after this point, context->func->close must be
  // called.

  *handle = context;

  return kEdcSensorFwUpdateLibResultOk;

err_exit:
  free(context);
  *handle = EDC_SENSOR_FW_UPDATE_LIB_IMPL_HANDLE_INVALID;
  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplCompleteWrite(
    EdcSensorFwUpdateLibImplHandle handle) {
  EdcSensorFwUpdateLibImplContext *context =
      (EdcSensorFwUpdateLibImplContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid context.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (context->state != kEdcSensorFwUpdateLibImplStateOpen) {
    DLOG_ERROR("Invalid state: %u.\n", context->state);
    return kEdcSensorFwUpdateLibResultFailedPrecondition;
  }

  if (context->func == NULL || context->func->close == NULL) {
    DLOG_ERROR("func->close is not registered for target component: %u.\n",
               context->target_component);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EdcSensorFwUpdateLibResult ret = context->func->close(context);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("func->close failed: %u\n", ret);
    return ret;
  }

  context->state = kEdcSensorFwUpdateLibImplStateClosed;

  if (context->func->complete_write != NULL) {
    ret = context->func->complete_write(context);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("func->complete_write failed: %u\n", ret);
      return ret;
    }
  }

  free(context);

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplCancelWrite(
    EdcSensorFwUpdateLibImplHandle handle) {
  EdcSensorFwUpdateLibImplContext *context =
      (EdcSensorFwUpdateLibImplContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid context.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;
  if (context->state == kEdcSensorFwUpdateLibImplStateOpen) {
    if (context->func == NULL || context->func->cancel_write == NULL) {
      DLOG_ERROR(
          "func->cancel_write is not registered for target component: %u.\n",
          context->target_component);
      return kEdcSensorFwUpdateLibResultInternal;
    }

    ret = context->func->close(context);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("func->close failed: %u\n", ret);
      return ret;
    }

    context->state = kEdcSensorFwUpdateLibImplStateClosed;
  }

  if (context->func != NULL && context->func->cancel_write != NULL) {
    ret = context->func->cancel_write(context);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("func->cancel_write failed: %u\n", ret);
      return ret;
    }
  }

  free(context);

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplWrite(
    EdcSensorFwUpdateLibImplHandle handle, EsfMemoryManagerHandle memory_handle,
    uint32_t size) {
  EdcSensorFwUpdateLibImplContext *context =
      (EdcSensorFwUpdateLibImplContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid context.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (context->state != kEdcSensorFwUpdateLibImplStateOpen) {
    DLOG_ERROR("Invalid state: %u.\n", context->state);
    return kEdcSensorFwUpdateLibResultFailedPrecondition;
  }

  if (context->func == NULL || context->func->write == NULL) {
    DLOG_ERROR("func->write is not registered for target component: %u.\n",
               context->target_component);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  void *mapped_address = NULL;
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerMap(memory_handle, NULL, size, &mapped_address);
  if (mem_ret != kEsfMemoryManagerResultSuccess || mapped_address == NULL) {
    DLOG_ERROR("EsfMemoryManagerMap failed: %u\n", mem_ret);
    return kEdcSensorFwUpdateLibResultResourceExhausted;
  }

  EdcSensorFwUpdateLibResult ret =
      context->func->write(context, mapped_address, size);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("func->write failed: %u\n", ret);
    goto unmap_and_exit;
  }

unmap_and_exit:
  mem_ret = EsfMemoryManagerUnmap(memory_handle, NULL);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    DLOG_ERROR("EsfMemoryManagerUnmap failed: %u\n", mem_ret);
    ret = kEdcSensorFwUpdateLibResultInternal;
  }
  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplErase(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *component_info) {
  const EdcSensorFwUpdateLibImplFunctions *func =
      GetFunctions(target_component);
  (void)target_device;  // Unused
  if (func == NULL) {
    DLOG_ERROR("Component %u is not supported.\n", target_component);
    return kEdcSensorFwUpdateLibResultUnimplemented;
  }
  if (func->erase == NULL) {
    DLOG_ERROR("Erasing component %u is not supported.\n", target_component);
    return kEdcSensorFwUpdateLibResultUnimplemented;
  }

  EdcSensorFwUpdateLibResult ret = func->erase(component_info);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("func->erase failed: %u\n", ret);
    return ret;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplGetMaxDataSizeOnce(
    EdcSensorFwUpdateLibImplHandle handle, uint32_t *size) {
  (void)handle;
  if (size == NULL) {
    DLOG_ERROR("size is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  // No limit on the size of the data to be written.
  *size = UINT32_MAX;
  return kEdcSensorFwUpdateLibResultOk;
}

/// @brief Get the item ID of the component info for the target component.
/// @param target_component [in] The target component.
/// @param target_device [in] The target device (not used).
/// @param is_active [in] Whether returning an item ID for active components
/// (true) or for inactive components (false).
/// @param item_id [out] The item ID of the component info.
/// @return kEdcSensorFwUpdateLibResultOk on success, or an error code on
/// failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImplGetPstorageItemId(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    bool is_active, EsfParameterStorageManagerItemID *item_id) {
  (void)target_device;

  if (item_id == NULL) {
    DLOG_ERROR("item_id is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  switch (target_component) {
    case kEdcSensorFwUpdateLibTargetAIModel:
      if (is_active) {
        *item_id = kEsfParameterStorageManagerItemFwMgrBinaryInfo1;
      } else {
        *item_id = kEsfParameterStorageManagerItemFwMgrBinaryInfo2;
      }
      break;

    case kEdcSensorFwUpdateLibTargetLoader:
    case kEdcSensorFwUpdateLibTargetFirmware:
    default:
      DLOG_ERROR("Unsupported target component: %u.\n", target_component);
      return kEdcSensorFwUpdateLibResultUnimplemented;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

bool EdcSensorFwUpdateLibImplCompareComponents(
    EdcSensorFwUpdateLibTarget target_component, const char *target_device,
    const EdcSensorFwUpdateLibComponentInfo *info_1,
    const EdcSensorFwUpdateLibComponentInfo *info_2) {
  (void)target_device;  // Unused

  if (info_1 == NULL || info_2 == NULL) {
    DLOG_ERROR("info_1 or info_2 is NULL.\n");
    return false;
  }

  switch (target_component) {
    case kEdcSensorFwUpdateLibTargetAIModel:
      // If the AI model bundle IDs are different, the components are
      // considered different.

      if (memcmp(&info_1->version[AI_MODEL_BUNDLE_ID_OFFSET],
                 &info_2->version[AI_MODEL_BUNDLE_ID_OFFSET],
                 AI_MODEL_BUNDLE_ID_SIZE) != 0) {
        return false;
      }

      break;

    case kEdcSensorFwUpdateLibTargetLoader:
    case kEdcSensorFwUpdateLibTargetFirmware:
    default:
      DLOG_ERROR("Unsupported target component: %u.\n", target_component);
      return false;
  }

  return true;
}
