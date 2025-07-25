/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/sensor_ai_lib_state.h"

#include <pthread.h>
#include <stdio.h>

struct SsfSensorLibAIDevSts {
  char dmy;
};

static pthread_mutex_t mtx_     = PTHREAD_MUTEX_INITIALIZER;
static SsfSensorLibState state_ = kSsfSensorLibStateStandby;
static struct SsfSensorLibAIDevSts dummy_;

struct SsfSensorLibAIDevSts* SsfSensorLibStateGet(SsfSensorLibState* p) {
  if (pthread_mutex_lock(&mtx_)) {
    return NULL;
  }
  *p = state_;
  return &dummy_;
}

bool SsfSensorLibStateRelease(struct SsfSensorLibAIDevSts* pLock) {
  if (pLock != &dummy_) {
    return false;
  }
  return !pthread_mutex_unlock(&mtx_);
}

bool SsfSensorLibStatePut(struct SsfSensorLibAIDevSts* pLock,
                          SsfSensorLibState next) {
  if (pLock != &dummy_) {
    return false;
  }
  state_ = next;
  return !pthread_mutex_unlock(&mtx_);
}

SsfSensorLibState SsfSensorLibStatePeek() {
  SsfSensorLibState ret;
  struct SsfSensorLibAIDevSts* p = SsfSensorLibStateGet(&ret);
  if (p) {
    SsfSensorLibStateRelease(p);
    return ret;
  }
  return kSsfSensorLibStateUnknown;
}

SsfSensorLibState SsfSensorLibGetState() {
  SsfSensorLibState ret = SsfSensorLibStatePeek();
  return ret;
}
