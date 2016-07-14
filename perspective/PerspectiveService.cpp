/*
 * Copyright 2015-2016 Preetam J. D'Souza
 * Copyright 2016 The Maru OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>

#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "PerspectiveService.h"
#include <lxc/lxccontainer.h>

#define DEBUG 1

namespace android {

/**
 * Name of the container under /data/maru/containers/
 */
static const char *CONTAINER = "jessie";

bool PerspectiveService::initContainer() {
    if (mContainer != NULL) {
        lxc_container_put(mContainer);
        mContainer = NULL;
    }

    mContainer = lxc_container_new(CONTAINER, NULL);
    if (mContainer == NULL) {
        ALOGW("can't initialize desktop container, container is NULL");
        return false;
    } else if (!mContainer->is_defined(mContainer)) {
        ALOGW("can't initialize desktop container, container is undefined");

        // drop reference, try again later
        lxc_container_put(mContainer);
        mContainer = NULL;

        return false;
    }

    return true;
}

PerspectiveService::PerspectiveService() : mContainer(NULL) {
    ALOGI("perspectived is starting...");

    initContainer();
}

PerspectiveService::~PerspectiveService() {
    if (mContainer != NULL) {
        lxc_container_put(mContainer);
        mContainer = NULL;
    }
}

bool PerspectiveService::startContainer(struct lxc_container *c) {
    if (!c->start(c, 0, NULL)) {
        ALOGD_IF(DEBUG, "liblxc returned false for start, possible false positive...");
        if (!c->is_running(c)) {
            return false;
        }
    }

    return true;
}

bool PerspectiveService::start() {
    ALOGD_IF(DEBUG, "running start()...");

    if (mContainer == NULL || !mContainer->is_defined(mContainer)) {
        ALOGW("start on uninitialized container detected, trying to init...");
        if (!initContainer()) {
            ALOGE("failed to start container, can't init container");
            return false;
        }
    }

    if (!mContainer->is_running(mContainer)) {
        if (!startContainer(mContainer)) {
            ALOGE("failed to start desktop");
            return false;
        }

        ALOGD_IF(DEBUG, "desktop GO!");
        ALOGD_IF(DEBUG, "container state: %s", mContainer->state(mContainer));
        ALOGD_IF(DEBUG, "container PID: %d", mContainer->init_pid(mContainer));
    } else {
        ALOGD("desktop already running, ignoring event...");
    }

    return true;
}

bool PerspectiveService::stop() {
    ALOGD_IF(DEBUG, "running stop()...");

    if (mContainer == NULL || !mContainer->is_defined(mContainer)) {
        ALOGW("stop on uninitialized container detected, trying to init...");
        if (!initContainer()) {
            ALOGE("failed to stop container, can't init container");
            return false;
        }
    }

    if (!mContainer->is_running(mContainer)) {
        // nothing to do
        return true;
    }

    return mContainer->stop(mContainer);
}

bool PerspectiveService::isRunning() {
    ALOGD_IF(DEBUG, "running isRunning()...");

    if (mContainer == NULL || !mContainer->is_defined(mContainer)) {
        ALOGW("check state on uninitialized container detected, trying to init...");
        if (!initContainer()) {
            ALOGE("failed to check container state, can't init container");
            return false;
        }
    }

    return mContainer->is_running(mContainer);
}

}; // namespace android

int main(void) {
    using namespace android;

    // start up the Binder IPC thread pool
    sp<ProcessState> ps(ProcessState::self());
    ps->setThreadPoolMaxThreadCount(1);
    ps->startThreadPool();

    sp<PerspectiveService> perspect = new PerspectiveService();

    // publish PerspectiveService
    sp<IServiceManager> sm(defaultServiceManager());
    sm->addService(String16(PerspectiveService::getServiceName()), perspect, false);

    // service Binder requests
    IPCThreadState::self()->joinThreadPool();

    // should never get here
    return -1;
}
