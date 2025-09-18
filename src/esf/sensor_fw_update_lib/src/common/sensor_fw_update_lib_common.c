/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sensor_fw_update_lib_common.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>  // For PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sensor_fw_update_lib_log.h"

/// @brief Equivalent to `mkdir -p dir_path` in bash.
/// Creates a directory at the specified path.
/// If the directory already exists, it does nothing.
/// @param dir_path
/// @return Returns `kEdcSensorFwUpdateLibResultOk` on success.
/// Returns `kEdcSensorFwUpdateLibResultInternal` on failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateDirectory(
    const char *dir_path) {
  if (dir_path == NULL) {
    DLOG_ERROR("dir_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  char path[PATH_MAX];
  char *p;

  int r = snprintf(path, sizeof(path), "%s", dir_path);
  if (r < 0 || (size_t)r >= sizeof(path)) {
    DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
               sizeof(path));
    return kEdcSensorFwUpdateLibResultInternal;
  }

  // Remove trailing slash if it exists
  size_t len = strlen(path);
  if (len > 1 && path[len - 1] == '/') {
    path[len - 1] = '\0';
  }

  // Create directories recursively
  for (p = path + 1;; p++) {
    if (*p == '/' || *p == '\0') {
      char temp = *p;
      *p        = '\0';
      if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        DLOG_ERROR("Failed to create directory: %s. (errno = %d)\n", path,
                   errno);
        return kEdcSensorFwUpdateLibResultInternal;
      }
      if (temp == '\0') {
        break;
      }
      *p = '/';
    }
  }

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibRemoveDirectory(
    const char *dir_path) {
  struct dirent *entry;
  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    if (errno == ENOENT) {
      DLOG_INFO("Directory does not exist: %s\n", dir_path);
      return kEdcSensorFwUpdateLibResultOk;
    } else {
      DLOG_ERROR("Failed to open directory: %s. (errno = %d)\n", dir_path,
                 errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char file_path[PATH_MAX];
    int r = snprintf(file_path, sizeof(file_path), "%s/%s", dir_path,
                     entry->d_name);
    if (r < 0 || (size_t)r >= sizeof(file_path)) {
      DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
                 sizeof(file_path));
      ret = kEdcSensorFwUpdateLibResultInternal;
      goto close_dir;
    }

    // Check if the entry is a directory using stat (more portable than d_type)
    struct stat entry_stat;
    if (lstat(file_path, &entry_stat) != 0) {
      DLOG_ERROR("stat(%s) failed. errno = %d\n", file_path, errno);
      ret = kEdcSensorFwUpdateLibResultInternal;
      goto close_dir;
    }

    if (S_ISDIR(entry_stat.st_mode)) {
      // The entry is a directory
      ret = EdcSensorFwUpdateLibRemoveDirectory(file_path);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        DLOG_ERROR("RemoveDirectory(%s) failed. (ret = %u)\n", file_path, ret);
        goto close_dir;
      }
    } else {
      // The entry is a file
      if (unlink(file_path) != 0) {
        DLOG_ERROR("unlink(%s) failed. (errno = %d)\n", file_path, errno);
        ret = kEdcSensorFwUpdateLibResultInternal;
        goto close_dir;
      }
    }
  }

close_dir:
  if (closedir(dir) != 0) {
    DLOG_ERROR("Failed to close directory: %s (errno = %d)\n", dir_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  if (ret == kEdcSensorFwUpdateLibResultOk) {
    if (rmdir(dir_path) != 0) {
      DLOG_ERROR("rmdir(%s) failed. (errno = %d)\n", dir_path, errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateEmptyFile(
    const char *file_path) {
  if (file_path == NULL) {
    DLOG_ERROR("file_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  FILE *fp = fopen(file_path, "w");
  if (fp == NULL) {
    DLOG_ERROR("Failed to open file: %s (errno = %d)\n", file_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EdcSensorFwUpdateLibResult ret = EdcSensorFwUpdateLibFflushAndFsync(fp);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    DLOG_ERROR("EdcSensorFwUpdateLibFflushAndFsync failed: %u\n", ret);
    goto close;
  }

close:
  if (fclose(fp) != 0) {
    DLOG_ERROR("Failed to close file: %s (errno = %d)\n", file_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibFflushAndFsync(FILE *fp) {
  if (fp == NULL) {
    DLOG_ERROR("fp is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  if (fflush(fp) != 0) {
    DLOG_ERROR("fflush failed. (errno = %d)\n", errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto exit;
  }

  if (fsync(fileno(fp)) != 0) {
    DLOG_ERROR("fsync failed. (errno = %d)\n", errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto exit;
  }

exit:
  return ret;
}
