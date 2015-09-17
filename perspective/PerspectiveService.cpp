#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <string.h>

#include <cutils/log.h>
#include <hardware_legacy/uevent.h>

#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "PerspectiveService.h"
#include "lxc/lxccontainer.h"

#define DEBUG 1

namespace android {

/**
 * Name of the container under /data/maru/containers/
 */
static const char *CONTAINER = "jessie";

PerspectiveService::PerspectiveService() : mContainer(NULL) {
    ALOGI("perspectived is starting...");
}

PerspectiveService::~PerspectiveService() {
    if (mContainer != NULL) {
        lxc_container_put(mContainer);
        mContainer = NULL;
    }
}

bool PerspectiveService::startContainer(struct lxc_container *c) {
    if (!c->start(c, 0, NULL)) {
        ALOGW("liblxc returned false for start, possible false positive...");
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

        ALOGI("desktop GO!");
        ALOGD("container state: %s", mContainer->state(mContainer));
        ALOGD("container PID: %d", mContainer->init_pid(mContainer));
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

bool PerspectiveService::isUeventFieldMatch(const char *s,
         const char *match, const int s_len) {
    if (s == NULL || match == NULL || s_len < 0) {
        return false;
    }

    /* uevents are '\0' delimited */
    const char *field = s;
    const char *end = s + s_len + 1;
    while (field < end) {
        // ALOGD("found field: %s", field);

        if (strstr(field, match) != NULL) {
            return true;
        }

        // move forward strlen plus '\0'
        field += strlen(field) + 1;
    }

    return false;
}

static const char *HDMI_UEVENT = "change@/devices/virtual/switch/hdmi";

void PerspectiveService::ueventPoll() {
    char uevent_desc[4096];
    struct pollfd fds[1];
    int timeout = -1;
    int err;

    ALOGI("starting uevent poll...");

    /* set up the kernel uevent netlink socket */
    uevent_init();

    memset(uevent_desc, 0, sizeof(uevent_desc));

    fds[0].fd = uevent_get_fd();
    fds[0].events = POLLIN;

    do {
        err = poll(fds, 1, timeout);

        if (err < 0) {
            ALOGE("poll error: %d", errno);
            continue;
        }

        if (fds[0].revents & POLLIN) {
            // keep last 2 zeroes to ensure double '\0' termination
            int len = uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);

            // for safety
            uevent_desc[len] = '\0';

            // ALOGI("****** got a uevent: %s", uevent_desc);

            if (!strcmp(uevent_desc, HDMI_UEVENT)) {
                if (isUeventFieldMatch(uevent_desc, "SWITCH_STATE=1", len)) {
                    start();
                }
            }
        }
    } while (1);
}

int PerspectiveService::run() {
    mContainer = lxc_container_new(CONTAINER, NULL);
    if (mContainer == NULL || !mContainer->is_defined(mContainer)) {
        ALOGE("can't initialize desktop container");

        // drop container reference so LXC can free
        lxc_container_put(mContainer);

        // fail fast
        return -1;
    }

    ueventPoll();

    // should never get here
    return -1;
}

}; // namespace android

int main(void) {
    using namespace android;

    // start up the Binder IPC thread pool
    sp<ProcessState> ps(ProcessState::self());
    ps->setThreadPoolMaxThreadCount(1);
    ps->startThreadPool();

    sp<PerspectiveService> perspect = new PerspectiveService();

    // TODO use init() and run() split to prevent clients connecting before init?

    // publish PerspectiveService
    sp<IServiceManager> sm(defaultServiceManager());
    sm->addService(String16(PerspectiveService::getServiceName()), perspect, false);

    // run service on main thread
    perspect->run();

    // should never get here
    return -1;
}
