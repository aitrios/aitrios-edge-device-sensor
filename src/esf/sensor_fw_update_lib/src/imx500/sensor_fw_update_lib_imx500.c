/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sensor_fw_update_lib_imx500.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "sensor_fw_update_lib_common.h"
#include "sensor_fw_update_lib_log.h"

// Note EdcSensorFwUpdateLibImx500DownloadHeader is the first 16 bytes of the
// download header. The size of the whole header is 32 bytes.
#define FPK_DOWNLOAD_HEADER_SIZE (0x20)
#define FPK_DOWNLOAD_FOOTER_SIZE (0x20)
// Additional size for MAC authentication extension in the footer.
#define FPK_DOWNLOAD_FOOTER_MAC_EXT_SIZE (0x20)

#define DOWNLOAD_HEADER_IDENTIFIER "4649"

typedef struct EdcSensorFwUpdateLibImx500DownloadHeader {
  char identifier[4];
  uint32_t data_size;
  uint16_t current_num;
  uint16_t total_num;
  uint8_t hdr_flg;  // not used in this sensor fw update lib
  uint8_t mac_auth_extention_enabled : 1;
  uint8_t reserved : 7;
  uint8_t reserved2[2];
} EdcSensorFwUpdateLibImx500DownloadHeader;

typedef struct EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo {
  size_t bytes_to_next_header;   // Number of bytes to the next header
  size_t remaining_header_size;  // Remaining size of the current header.
  size_t header_count;           // Number of headers read so far
  bool is_all_headers_read;      // True if all headers are read.
  EdcSensorFwUpdateLibImx500DownloadHeader header;  // buffer for the current
                                                    // header being read

  // Total size of the FPK data. Reading the download headers is to calculate
  // this value.
  size_t *fpk_data_size;
} EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo;

#define IMAGE_PACKET_HEADER_VERSION_LENGTH (0x10)
typedef struct EdcSensorFwUpdateLibImx500ImagePacketHeader {
  uint8_t reserved[16];
  char version[IMAGE_PACKET_HEADER_VERSION_LENGTH];  // Without null
                                                     // terminator
} EdcSensorFwUpdateLibImx500ImagePacketHeader;

typedef struct EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo {
  size_t bytes_to_next_header;   // Number of bytes to the next header
  size_t remaining_header_size;  // Remaining size of the current header.
  bool is_read;                  // True if the image packet header is read.
  EdcSensorFwUpdateLibImx500ImagePacketHeader header;  // buffer for the header
  // The version written in the header. Reading the image packet
  // header is to get this value.
  char *version;
  size_t version_size;  // Size of the version buffer
} EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo;
typedef struct EdcSensorFwUpdateLibImx500ReadHeadersCommonInfo {
  const uint8_t *data_current;
  const uint8_t *data_end;
  size_t bytes_to_next_loop;
} EdcSensorFwUpdateLibImx500ReadHeadersCommonInfo;

static bool IsDigit(char c) { return ('0' <= c && c <= '9'); }

/// @brief Verify the version string.
/// @param version The version string to verify.
/// @param version_size The size of the version string.
/// @return true if the version string is valid, false otherwise.
/// @note The version string is expected to be a string of digits only, with
/// no null terminator.
static bool VerifyVersion(const char *version, size_t version_size) {
  if (version == NULL || version_size == 0) {
    DLOG_ERROR("version or version_size is NULL or zero.\n");
    return false;
  }

  for (size_t i = 0; i < version_size; i++) {
    if (!IsDigit(version[i])) return false;
  }

  return true;
}

/// @brief Verify the download header.
/// @param header
/// @return true if the header is valid, false otherwise.
static bool VerifyDownloadHeader(
    const EdcSensorFwUpdateLibImx500DownloadHeader *header) {
  if (header == NULL) {
    DLOG_ERROR("header is NULL.\n");
    return false;
  }
  if (strncmp(header->identifier, DOWNLOAD_HEADER_IDENTIFIER,
              sizeof(header->identifier)) != 0) {
    DLOG_ERROR("Invalid identifier: %.*s.\n", (int)sizeof(header->identifier),
               header->identifier);
    return false;
  }

  return true;
}

/// @brief
/// @param common_info
/// @param info
/// @return
static EdcSensorFwUpdateLibResult ParseDownloadHeader(
    EdcSensorFwUpdateLibImx500ReadHeadersCommonInfo *common_info,
    EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo *info) {
  if (common_info == NULL || info == NULL) {
    DLOG_ERROR("common_info or info is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (info->is_all_headers_read) {
    DLOG_DEBUG("All download headers have been read. Do nothing\n");
    return kEdcSensorFwUpdateLibResultOk;
  }

  if (common_info->data_current >= common_info->data_end) {
    DLOG_ERROR("data_current >= data_end\n");
    return kEdcSensorFwUpdateLibResultFailedPrecondition;
  }

  DLOG_DEBUG("bytes_to_next_header: %zu\n", info->bytes_to_next_header);
  DLOG_DEBUG("remaining_header_size: %zu\n", info->remaining_header_size);
  DLOG_DEBUG("header_count: %zu\n", info->header_count);
  DLOG_DEBUG("reaming_data_size: %td\n",
             common_info->data_end - common_info->data_current);

  if (info->bytes_to_next_header == 0) {
    if (info->remaining_header_size > 0) {
      size_t read_size =
          MIN((size_t)(common_info->data_end - common_info->data_current),
              info->remaining_header_size);

      uint8_t *header_ptr =
          ((uint8_t *)&info->header) +
          (sizeof(info->header) - info->remaining_header_size);
      memcpy(header_ptr, common_info->data_current, read_size);
      info->remaining_header_size -= read_size;
      common_info->data_current += read_size;
      *info->fpk_data_size += read_size;
    }

    if (info->remaining_header_size == 0) {
      // Header is fully read.
      info->header_count++;
      if (!VerifyDownloadHeader(&info->header) ||
          (info->header_count != info->header.current_num)) {
        DLOG_ERROR("Invalid download header.\n");
        return kEdcSensorFwUpdateLibResultInvalidArgument;
      }

      info->is_all_headers_read =
          (info->header.current_num == info->header.total_num);

      info->bytes_to_next_header =
          FPK_DOWNLOAD_HEADER_SIZE - sizeof(info->header) +
          info->header.data_size + FPK_DOWNLOAD_FOOTER_SIZE;
      if (info->header.mac_auth_extention_enabled) {
        info->bytes_to_next_header += FPK_DOWNLOAD_FOOTER_MAC_EXT_SIZE;
      }

      *info->fpk_data_size += info->bytes_to_next_header;

      // Reset the header for the next read.
      info->remaining_header_size = sizeof(info->header);
      memset(&info->header, 0, sizeof(info->header));
    }
  }

  common_info->bytes_to_next_loop =
      MIN((size_t)(common_info->data_end - common_info->data_current),
          info->bytes_to_next_header);
  DLOG_DEBUG("bytes_to_next_loop: %zu\n", common_info->bytes_to_next_loop);
  DLOG_DEBUG("bytes_to_next_header: %zu\n", info->bytes_to_next_header);
  DLOG_DEBUG("reaming_data_size: %td\n",
             common_info->data_end - common_info->data_current);
  DLOG_DEBUG("is_all_headers_read: %d\n", info->is_all_headers_read);
  DLOG_DEBUG("---------------------------------\n");
  info->bytes_to_next_header -= common_info->bytes_to_next_loop;

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult ParseImagePacketHeader(
    EdcSensorFwUpdateLibImx500ReadHeadersCommonInfo *common_info,
    EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo *info) {
  if (common_info == NULL || info == NULL) {
    DLOG_ERROR("common_info or info is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (info->is_read) return kEdcSensorFwUpdateLibResultOk;

  if (common_info->data_current >= common_info->data_end) {
    DLOG_ERROR("data_current >= data_end\n");
    return kEdcSensorFwUpdateLibResultFailedPrecondition;
  }

  DLOG_DEBUG("bytes_to_next_header: %zu\n", info->bytes_to_next_header);
  DLOG_DEBUG("remaining_header_size: %zu\n", info->remaining_header_size);
  DLOG_DEBUG("reaming_data_size: %td\n",
             common_info->data_end - common_info->data_current);

  if (info->bytes_to_next_header == 0) {
    if (info->remaining_header_size > 0) {
      size_t read_size =
          MIN((size_t)(common_info->data_end - common_info->data_current),
              info->remaining_header_size);

      uint8_t *header_ptr =
          ((uint8_t *)&info->header) +
          (sizeof(info->header) - info->remaining_header_size);
      memcpy(header_ptr, common_info->data_current, read_size);
      info->remaining_header_size -= read_size;
      common_info->data_current += read_size;
    }

    if (info->remaining_header_size == 0) {
      // Header is fully read.
      if (!VerifyVersion(info->header.version, sizeof(info->header.version))) {
        DLOG_ERROR("Invalid version in the image packet header.\n");
        return kEdcSensorFwUpdateLibResultInvalidArgument;
      }

      // info->header.version does not have the null terminator.
      size_t version_end =
          MIN(info->version_size - 1, sizeof(info->header.version));
      strncpy(info->version, info->header.version, version_end);
      info->version[version_end] = '\0';

      DLOG_INFO("Version: %s\n", info->version);
      info->is_read = true;
    }
  }

  common_info->bytes_to_next_loop =
      MIN((size_t)(common_info->data_end - common_info->data_current),
          info->bytes_to_next_header);
  DLOG_DEBUG("bytes_to_next_loop: %zu\n", common_info->bytes_to_next_loop);
  DLOG_DEBUG("bytes_to_next_header: %zu\n", info->bytes_to_next_header);
  DLOG_DEBUG("reaming_data_size: %td\n",
             common_info->data_end - common_info->data_current);
  DLOG_DEBUG("---------------------------------\n");
  info->bytes_to_next_header -= common_info->bytes_to_next_loop;

  return kEdcSensorFwUpdateLibResultOk;
}

static void InitReadDownloadHeadersInfo(
    EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo *info) {
  if (info == NULL) return;
  info->bytes_to_next_header  = 0;
  info->remaining_header_size = sizeof(info->header);
  info->header_count          = 0;
  info->is_all_headers_read   = false;
  memset(&info->header, 0, sizeof(info->header));
  info->fpk_data_size = NULL;  // Set this after the context is created
}

static void InitReadImagePacketHeaderInfo(
    EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo *info) {
  if (info == NULL) return;
  info->bytes_to_next_header  = FPK_DOWNLOAD_HEADER_SIZE;
  info->remaining_header_size = sizeof(info->header);
  info->is_read               = false;
  memset(&info->header, 0, sizeof(info->header));
  info->version      = NULL;  // Set this after the context is created
  info->version_size = 0;     // Set this after the context is created
}

/// @brief Update context->fpk_data_size and context->version from the info in
/// the download/package header.
/// @param context
/// @param data
/// @param size
/// @return
/// @note context->bytes_to_next_header == 0 means header is currently being
/// read.
/// Data structure:
///  +----------------------+
///  | Download Header      | 32 bytes
///  +----------------------+
///  | Image Packet Header  | 32 bytes
///  +----------------------+
///  | Data                 | (download_header->data_size - 32 + footer size)
///  +----------------------+
///  | Download Header      | 32 bytes
///  +----------------------+
///  | Data                 | (download_header->data_size + footer size) bytes
///  +----------------------+
///  ...
///  +----------------------+
///  | Download Header      | 32 bytes
///  +----------------------+
///  | Data                 | (download_header->data_size + footer size) bytes
///  +----------------------+
/// The footer size is 64 bytes if MAC authentication extension is enabled,
/// otherwise it is 32 bytes.
static EdcSensorFwUpdateLibResult ReadHeaders(
    EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo *download_headers,
    EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo *image_packet_header,
    const uint8_t *data, size_t data_size) {
  if (download_headers == NULL || image_packet_header == NULL || data == NULL) {
    DLOG_ERROR("context or mapped_address is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  typedef enum {
    kHeaderTypeDownload = 0,
    kHeaderTypeImagePacket,
    kHeaderTypeNum,
  } HeaderType;

  for (HeaderType header_type = 0; header_type < kHeaderTypeNum;
       ++header_type) {
    EdcSensorFwUpdateLibImx500ReadHeadersCommonInfo common = {
        .data_current       = data,
        .data_end           = data + data_size,
        .bytes_to_next_loop = 0,
    };

    while (common.data_current < common.data_end) {
      // The below will be updated by ParseDownloadHeader.
      common.bytes_to_next_loop = common.data_end - common.data_current;

      if (header_type == kHeaderTypeDownload) {
        EdcSensorFwUpdateLibResult ret =
            ParseDownloadHeader(&common, download_headers);
        if (ret != kEdcSensorFwUpdateLibResultOk) {
          DLOG_ERROR("ParseDownloadHeader failed: %u\n", ret);
          return ret;
        }

      } else if (header_type == kHeaderTypeImagePacket) {
        EdcSensorFwUpdateLibResult ret =
            ParseImagePacketHeader(&common, image_packet_header);
        if (ret != kEdcSensorFwUpdateLibResultOk) {
          DLOG_ERROR("ParseImagePacketHeader failed: %u\n", ret);
          return ret;
        }
      }
      common.data_current += common.bytes_to_next_loop;

    }  // while (remaining_size > 0)
  }  // for (int i = 0; i < kHeaderTypeNum; i++)

  return kEdcSensorFwUpdateLibResultOk;
}

typedef enum EdcSensorFwUpdateLibImx500WriteAiModelState {
  kEdcSensorFwUpdateLibImx500WriteAiModelStateWritingFpk,
  kEdcSensorFwUpdateLibImx500WriteAiModelStateWritingInfo,
} EdcSensorFwUpdateLibImx500WriteAiModelState;

typedef struct EdcSensorFwUpdateLibImx500AiModelContext {
  char fpk_file_path[PATH_MAX];
  char network_info_file_path[PATH_MAX];
  FILE *fp;
  EdcSensorFwUpdateLibImx500WriteAiModelState state;
  EdcSensorFwUpdateLibImx500ReadDownloadHeadersInfo download_headers;
  EdcSensorFwUpdateLibImx500ReadImagePacketHeaderInfo image_packet_header;
  size_t fpk_data_size;
  size_t total_written_size;
} EdcSensorFwUpdateLibImx500AiModelContext;

static bool IsValidContext(EdcSensorFwUpdateLibImx500AiModelContext *context) {
  if (context == EDC_SENSOR_FW_UPDATE_LIB_IMX500_AI_MODEL_HANDLE_INVALID) {
    DLOG_ERROR("context is NULL.\n");
    return false;
  }

  return true;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelOpen(
    const char *fpk_file_path, const char *network_info_file_path,
    char *version, size_t version_size,
    EdcSensorFwUpdateLibImx500AiModelHandle *handle) {
  if (fpk_file_path == NULL || network_info_file_path == NULL ||
      handle == NULL) {
    DLOG_ERROR("fpk_file_path or network_info_file_path or handle is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EdcSensorFwUpdateLibImx500AiModelContext *context =
      (EdcSensorFwUpdateLibImx500AiModelContext *)malloc(
          sizeof(EdcSensorFwUpdateLibImx500AiModelContext));
  if (context == NULL) {
    DLOG_ERROR("Failed to allocate memory for context.\n");
    return kEdcSensorFwUpdateLibResultResourceExhausted;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  int r = snprintf(context->fpk_file_path, sizeof(context->fpk_file_path), "%s",
                   fpk_file_path);
  if (r < 0 || (size_t)r >= sizeof(context->fpk_file_path)) {
    DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
               sizeof(context->fpk_file_path));
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto err_exit;
  }

  r = snprintf(context->network_info_file_path,
               sizeof(context->network_info_file_path), "%s",
               network_info_file_path);
  if (r < 0 || (size_t)r >= sizeof(context->network_info_file_path)) {
    DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
               sizeof(context->network_info_file_path));
    ret = kEdcSensorFwUpdateLibResultInvalidArgument;
    goto err_exit;
  }

  context->fp = fopen(context->fpk_file_path, "wb");
  if (context->fp == NULL) {
    DLOG_ERROR("Failed to open file: %s (errno = %d)\n", context->fpk_file_path,
               errno);
    ret = kEdcSensorFwUpdateLibResultResourceExhausted;
    goto err_exit;
  }

  context->fpk_data_size = 0;

  // download_headers
  InitReadDownloadHeadersInfo(&context->download_headers);
  context->download_headers.fpk_data_size = &context->fpk_data_size;

  // image_packet_header
  InitReadImagePacketHeaderInfo(&context->image_packet_header);
  context->image_packet_header.version      = version;
  context->image_packet_header.version_size = version_size;

  context->total_written_size = 0;
  context->state = kEdcSensorFwUpdateLibImx500WriteAiModelStateWritingFpk;

  *handle = context;

  return kEdcSensorFwUpdateLibResultOk;

err_exit:
  free(context);
  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelWrite(
    EdcSensorFwUpdateLibImx500AiModelHandle handle, const uint8_t *data,
    size_t size) {
  if (data == NULL || size == 0) {
    DLOG_ERROR("Invalid arguments. data: %p, size: %zu\n", data, size);
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  EdcSensorFwUpdateLibImx500AiModelContext *context =
      (EdcSensorFwUpdateLibImx500AiModelContext *)handle;
  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid context\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  if (context->fp == NULL) {
    DLOG_ERROR("File pointer is NULL. context->fp: %p\n", context->fp);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;
  if (context->state ==
      kEdcSensorFwUpdateLibImx500WriteAiModelStateWritingFpk) {
    ret = ReadHeaders(&context->download_headers, &context->image_packet_header,
                      data, size);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("ReadDownloadHeader failed: %u\n", ret);
      return ret;
    }

    if (context->fpk_data_size < context->total_written_size + size) {
      size_t write_size = context->fpk_data_size - context->total_written_size;
      if (write_size > 0) {
        size_t written_size = fwrite(data, 1, write_size, context->fp);
        if (written_size != write_size) {
          DLOG_ERROR(
              "fwrite failed. Target size: %zu, written size: %zu (errno = "
              "%d)\n",
              write_size, written_size, errno);
          return kEdcSensorFwUpdateLibResultInternal;
        }
        context->total_written_size += write_size;
        data = data + write_size;
        size -= write_size;
      }

      ret = EdcSensorFwUpdateLibFflushAndFsync(context->fp);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        DLOG_ERROR("EdcSensorFwUpdateLibFflushAndFsync failed: %u\n", ret);
        return ret;
      }

      if (fclose(context->fp) != 0) {
        DLOG_ERROR("Failed to close file: %s (errno = %d)\n",
                   context->fpk_file_path, errno);
        return kEdcSensorFwUpdateLibResultInternal;
      }

      const char *file_path = context->network_info_file_path;
      context->fp           = fopen(file_path, "wb");
      if (context->fp == NULL) {
        DLOG_ERROR("Failed to open file: %s (errno = %d)\n", file_path, errno);
        return kEdcSensorFwUpdateLibResultResourceExhausted;
      }
      context->state = kEdcSensorFwUpdateLibImx500WriteAiModelStateWritingInfo;
    }
  }

  size_t written_size = fwrite(data, 1, size, context->fp);
  if (written_size != size) {
    DLOG_ERROR(
        "fwrite failed. Target size: %zu, written size: %zu (errno = %d)\n",
        size, written_size, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }
  context->total_written_size += written_size;

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibImx500AiModelClose(
    EdcSensorFwUpdateLibImx500AiModelHandle handle) {
  EdcSensorFwUpdateLibImx500AiModelContext *context =
      (EdcSensorFwUpdateLibImx500AiModelContext *)handle;

  if (!IsValidContext(context)) {
    DLOG_ERROR("Invalid context\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (context->fp != NULL) {
    EdcSensorFwUpdateLibResult ret =
        EdcSensorFwUpdateLibFflushAndFsync(context->fp);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      DLOG_ERROR("EdcSensorFwUpdateLibFflushAndFsync failed: %u\n", ret);
      return ret;
    }

    if (fclose(context->fp) != 0) {
      DLOG_ERROR("Failed to close file: %s (errno = %d)\n",
                 context->fpk_file_path, errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }
    context->fp = NULL;
  }
  free(context);
  return kEdcSensorFwUpdateLibResultOk;
}
