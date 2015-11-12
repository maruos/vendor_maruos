/*
 * Copyright (C) 2015 Preetam D'Souza
 *
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

PerspectiveService::PerspectiveService() : mContainer(NULL) {
    ALOGI("perspectived is starting...");

    mContainer = lxc_container_new(CONTAINER, NULL);
    if (mContainer == NULL || !mContainer->is_defined(mContainer)) {
        LOG_ALWAYS_FATAL("can't initialize desktop container");

        // drop container reference so LXC can free
        lxc_container_put(mContainer);

        // fail fast
        exit(1);
    }
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

    if (!mContainer->is_running(mContainer)) {
        // nothing to do
        return true;
    }

    return mContainer->stop(mContainer);
}

bool PerspectiveService::isRunning() {
    ALOGD_IF(DEBUG, "running isRunning()...");

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
