/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sensor_fw_update_lib_common.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>   // For O_RDONLY, O_DIRECTORY
#include <limits.h>  // For PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sensor_fw_update_lib_log.h"

/// @brief Equivalent to `fsync` on the directory specified by `dir_path`.
/// @param dir_path
/// @return Returns `kEdcSensorFwUpdateLibResultOk` on success.
/// Returns `kEdcSensorFwUpdateLibResultInternal` on failure.
static EdcSensorFwUpdateLibResult FsyncDirectory(const char *dir_path) {
  int dir_fd = open(dir_path, O_RDONLY | O_DIRECTORY);
  if (dir_fd < 0) {
    SEND_DLOG_ERROR("Failed to open directory: %s (errno = %d)\n", dir_path,
                    errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;
  if (fsync(dir_fd) != 0) {
    SEND_DLOG_ERROR("fsync(%s) failed. (errno = %d)\n", dir_path, errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto close;
  }

close:
  if (close(dir_fd) != 0) {
    SEND_DLOG_WARNING("Failed to close directory: %s (errno = %d)\n", dir_path,
                      errno);
  }

  return ret;
}

/// @brief Equivalent to `mkdir -p dir_path` in bash.
/// Creates a directory at the specified path.
/// If the directory already exists, it does nothing.
/// @param dir_path
/// @return Returns `kEdcSensorFwUpdateLibResultOk` on success.
/// Returns `kEdcSensorFwUpdateLibResultInternal` on failure.
EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateDirectory(
    const char *dir_path) {
  if (dir_path == NULL) {
    SEND_DLOG_ERROR("dir_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  char path[PATH_MAX];
  char *p;

  int r = snprintf(path, sizeof(path), "%s", dir_path);
  if (r < 0 || (size_t)r >= sizeof(path)) {
    SEND_DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
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
        SEND_DLOG_ERROR("Failed to create directory: %s. (errno = %d)\n", path,
                        errno);
        return kEdcSensorFwUpdateLibResultInternal;
      }

      EdcSensorFwUpdateLibResult ret =
          EdcSensorFwUpdateLibFsyncParentDirectory(path);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        SEND_DLOG_ERROR(
            "EdcSensorFwUpdateLibFsyncParentDirectory(%s) failed. (ret = %u)\n",
            path, ret);
        return ret;
      }

      if (temp == '\0') {
        break;
      }
      *p = '/';
    }
  }

  return kEdcSensorFwUpdateLibResultOk;
}

static EdcSensorFwUpdateLibResult RemoveDirectoryInternal(const char *dir_path,
                                                          bool internal_call) {
  struct dirent *entry;
  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    if (errno == ENOENT) {
      SEND_DLOG_INFO("Directory does not exist: %s\n", dir_path);
      return kEdcSensorFwUpdateLibResultOk;
    } else {
      SEND_DLOG_ERROR("Failed to open directory: %s. (errno = %d)\n", dir_path,
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
      SEND_DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
                      sizeof(file_path));
      ret = kEdcSensorFwUpdateLibResultInternal;
      goto close_dir;
    }

    // Check if the entry is a directory using stat (more portable than d_type)
    struct stat entry_stat;
    if (lstat(file_path, &entry_stat) != 0) {
      SEND_DLOG_ERROR("stat(%s) failed. errno = %d\n", file_path, errno);
      ret = kEdcSensorFwUpdateLibResultInternal;
      goto close_dir;
    }

    if (S_ISDIR(entry_stat.st_mode)) {
      // The entry is a directory
      ret = RemoveDirectoryInternal(file_path, true);
      if (ret != kEdcSensorFwUpdateLibResultOk) {
        SEND_DLOG_ERROR("RemoveDirectoryInternal(%s) failed. (ret = %u)\n",
                        file_path, ret);
        goto close_dir;
      }
    } else {
      // The entry is a file
      if (unlink(file_path) != 0) {
        SEND_DLOG_ERROR("unlink(%s) failed. (errno = %d)\n", file_path, errno);
        ret = kEdcSensorFwUpdateLibResultInternal;
        goto close_dir;
      }
    }
  }

  ret = FsyncDirectory(dir_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    SEND_DLOG_ERROR("FsyncDirectory(%s) failed. (ret = %u)\n", dir_path, ret);
    goto close_dir;
  }

close_dir:
  if (closedir(dir) != 0) {
    SEND_DLOG_ERROR("Failed to close directory: %s (errno = %d)\n", dir_path,
                    errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  if (ret != kEdcSensorFwUpdateLibResultOk) return ret;

  if (rmdir(dir_path) != 0) {
    SEND_DLOG_ERROR("rmdir(%s) failed. (errno = %d)\n", dir_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  if (!internal_call) {
    ret = EdcSensorFwUpdateLibFsyncParentDirectory(dir_path);
    if (ret != kEdcSensorFwUpdateLibResultOk) {
      SEND_DLOG_ERROR(
          "EdcSensorFwUpdateLibFsyncParentDirectory(%s) failed. (ret = %u)\n",
          dir_path, ret);
      return ret;
    }
  }

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibRemoveDirectory(
    const char *dir_path) {
  return RemoveDirectoryInternal(dir_path, false);
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibCreateEmptyFile(
    const char *file_path) {
  if (file_path == NULL) {
    SEND_DLOG_ERROR("file_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  FILE *fp = fopen(file_path, "w");
  if (fp == NULL) {
    SEND_DLOG_ERROR("Failed to open file: %s (errno = %d)\n", file_path, errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  EdcSensorFwUpdateLibResult ret = EdcSensorFwUpdateLibFflushAndFsync(fp);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    SEND_DLOG_ERROR("EdcSensorFwUpdateLibFflushAndFsync failed: %u\n", ret);
    goto close;
  }

close:
  if (fclose(fp) != 0) {
    SEND_DLOG_ERROR("Failed to close file: %s (errno = %d)\n", file_path,
                    errno);
    return kEdcSensorFwUpdateLibResultInternal;
  }

  if (ret != kEdcSensorFwUpdateLibResultOk) return ret;

  ret = EdcSensorFwUpdateLibFsyncParentDirectory(file_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    SEND_DLOG_ERROR("EdcSensorFwUpdateLibFsyncParentDirectory failed: %u\n",
                    ret);
    return ret;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibRemoveFileSafely(
    const char *file_path) {
  if (file_path == NULL) {
    SEND_DLOG_ERROR("file_path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  if (remove(file_path) != 0) {
    if (errno == ENOENT) {
      SEND_DLOG_INFO("File does not exist: %s\n", file_path);
      // No need to fsync parent directory if the file does not exist
      return kEdcSensorFwUpdateLibResultOk;
    } else {
      SEND_DLOG_ERROR("Failed to delete file: %s (errno = %d)\n", file_path,
                      errno);
      return kEdcSensorFwUpdateLibResultInternal;
    }
  }

  EdcSensorFwUpdateLibResult ret =
      EdcSensorFwUpdateLibFsyncParentDirectory(file_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    SEND_DLOG_ERROR("EdcSensorFwUpdateLibFsyncParentDirectory failed: %u\n",
                    ret);
    return ret;
  }

  return kEdcSensorFwUpdateLibResultOk;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibFflushAndFsync(FILE *fp) {
  if (fp == NULL) {
    SEND_DLOG_ERROR("fp is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  EdcSensorFwUpdateLibResult ret = kEdcSensorFwUpdateLibResultOk;

  if (fflush(fp) != 0) {
    SEND_DLOG_ERROR("fflush failed. (errno = %d)\n", errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto exit;
  }

  if (fsync(fileno(fp)) != 0) {
    SEND_DLOG_ERROR("fsync failed. (errno = %d)\n", errno);
    ret = kEdcSensorFwUpdateLibResultInternal;
    goto exit;
  }

exit:
  return ret;
}

EdcSensorFwUpdateLibResult EdcSensorFwUpdateLibFsyncParentDirectory(
    const char *path) {
  if (path == NULL) {
    SEND_DLOG_ERROR("path is NULL.\n");
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  char dir_path[PATH_MAX];
  int r = snprintf(dir_path, sizeof(dir_path), "%s", path);
  if (r < 0 || (size_t)r >= sizeof(dir_path)) {
    SEND_DLOG_ERROR("snprintf failed. ret = %d (buffer size = %zu)\n", r,
                    sizeof(dir_path));
    return kEdcSensorFwUpdateLibResultInternal;
  }

  size_t len = strnlen(dir_path, sizeof(dir_path));
  if (len == 0 || len == sizeof(dir_path)) {
    SEND_DLOG_ERROR("Invalid directory path: %s\n", dir_path);
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }

  // Remove trailing slashes if they exist
  while (dir_path[len - 1] == '/') {
    dir_path[len - 1] = '\0';
    --len;
    if (len == 0) {
      SEND_DLOG_ERROR("Invalid directory path: %s\n", dir_path);
      return kEdcSensorFwUpdateLibResultInvalidArgument;
    }
  }

  char *last_slash = strrchr(dir_path, '/');
  if (last_slash == NULL) {
    SEND_DLOG_ERROR("No directory component in path: %s\n", path);
    return kEdcSensorFwUpdateLibResultInvalidArgument;
  }
  if (last_slash == dir_path) {
    // The parent directory is the root directory
    last_slash[1] = '\0';  // Keep the leading '/'
  } else {
    *last_slash = '\0';  // Terminate to get the parent directory path
  }

  EdcSensorFwUpdateLibResult ret = FsyncDirectory(dir_path);
  if (ret != kEdcSensorFwUpdateLibResultOk) {
    SEND_DLOG_ERROR("FsyncDirectory(%s) failed. (ret = %u)\n", dir_path, ret);
    return ret;
  }

  return kEdcSensorFwUpdateLibResultOk;
}
