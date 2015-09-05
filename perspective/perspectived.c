#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include <cutils/log.h>
#include <hardware_legacy/uevent.h>

#include "lxc/lxccontainer.h"

#define CONTAINER "jessie"

struct perspectived_state {
    int hdmi_switch;        /* last hdmi switch state */
    struct lxc_container *container;
};
static struct perspectived_state state;

static int is_running(struct lxc_container *container) {
    return container != NULL && container->is_running(container);
}

static void uevent_poll() {
    char uevent_desc[4096];
    struct pollfd fds[1];
    int timeout = -1;
    int err;

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
            /* keep last 2 zeroes to ensure double 0 termination */
            int len = uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);
            // ALOGI("****** got a uevent: %s", uevent_desc);

            if (!strcmp(uevent_desc, "change@/devices/virtual/switch/hdmi")) {
                ALOGI("****** Got hdmi switch event!");

                /* TODO: parse uevent for switch state */

                if (!is_running(state.container)) {
                    if (!state.container->start(state.container, 0, NULL)) {
                        ALOGW("liblxc returned false for start, possible false positive...");
                        if (!is_running(state.container)) {
                            ALOGE("failed to start desktop");
                            continue;
                        }
                    }

                    ALOGI("desktop GO!");
                } else {
                    ALOGD("desktop already running, ignoring event...");
                }
            }
        }
    } while (1);
}

int main(void) {

    state.container = lxc_container_new(CONTAINER, NULL);
    if (state.container == NULL || !state.container->is_defined(state.container)) {
        ALOGE("can't initialize desktop container");
        return -1;
    }

    ALOGI("starting uevent poll...");
    uevent_poll();
   
    // never reaches here
    return 0;
}
