/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensor_main.h"

EsfSensorErrCode EsfSensorInit(void) {
  return kEsfSensorOk;
}

EsfSensorErrCode EsfSensorExit(void) {
  return kEsfSensorOk;
}

void EsfSensorPowerOFF(void) {
  // Stub implementation
}

EsfSensorErrCode EsfSensorUtilitySetupFiles(void) {
  return kEsfSensorOk;
}

EsfSensorErrCode EsfSensorUtilityVerifyFiles(void) {
  return kEsfSensorOk;
}

#if defined(CONFIG_SENSOR_TARGET_T4)
#define IMX500_DNN_FILE_DIR "/misc/imx500/ai_models/"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// PATH_MAX should be defined in limits.h, but provide fallback for environments
// where it's not
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int RemoveDirRecursive(const char *path) {
  if (path == NULL) {
    return -1;
  }
  DIR *d = opendir(path);
  int r = -1;
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p = readdir(d))) {
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      char buf[PATH_MAX];
      snprintf(buf, sizeof(buf), "%s/%s", path, p->d_name);
      struct stat statbuf;
      if (!stat(buf, &statbuf)) {
        if (S_ISDIR(statbuf.st_mode)) {
          r = RemoveDirRecursive(buf);
        } else {
          r = remove(buf);
        }
      }
    }
    closedir(d);
  }
  if (!r) {
    r = rmdir(path);
  }
  return r;
}

static int RemoveDirRecursiveExceptPreinstall(const char *path) {
  if (path == NULL) {
    return -1;
  }
  DIR *d = opendir(path);
  int r = -1;
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p = readdir(d))) {
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      char buf[PATH_MAX];
      snprintf(buf, sizeof(buf), "%s/%s", path, p->d_name);
      struct stat statbuf;
      if (!stat(buf, &statbuf)) {
        if (S_ISDIR(statbuf.st_mode)) {
          // Keep directories with "_preinstall" in their name
          if (strstr(p->d_name, "_preinstall") != NULL) {
            continue;
          }
          r = RemoveDirRecursive(buf);
        } else {
          r = remove(buf);
        }
      }
    }
    closedir(d);
  }
  return r;
}
#endif  // defined(CONFIG_SENSOR_TARGET_T4)

EsfSensorErrCode EsfSensorUtilityResetFiles(void) {
#if defined(CONFIG_SENSOR_TARGET_T4)
  // Remove everything under IMX500_DNN_FILE_DIR except
  // directories with "_preinstall" in their name
  int ret = RemoveDirRecursiveExceptPreinstall(IMX500_DNN_FILE_DIR);
  if (ret != 0) {
    return kEsfSensorFail;
  }
#endif  // defined(CONFIG_SENSOR_TARGET_T4)
  return kEsfSensorOk;
}
