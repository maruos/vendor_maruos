#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include <cutils/log.h>
#include <hardware_legacy/uevent.h>

struct perspectived_state {
    int hdmi_switch;        /* last hdmi switch state */
    int desktop_running;    /* is desktop container running? */ 
};
static struct perspectived_state state;

static void uevent_poll() {
    char uevent_desc[4096];
    struct pollfd fds[1];
    int timeout = -1;
    int err;

    ALOGD("before uevent_init");

    /* set up the kernel uevent netlink socket */
    uevent_init();

    ALOGD("after uevent_init");

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

                if (!state.desktop_running) {
                    err = system("/system/xbin/mcontainer-start.sh -d");
                    if (err < 0) {
                        ALOGE("failed to start desktop");
                        continue;
                    }

                    ALOGI("desktop GO!");
                    state.desktop_running = 1;
                }
            }
        }
    } while (1);
}

int main(void) {

    ALOGI("starting uevent poll...");

    uevent_poll();
   
    // never reaches here
    return 0;
}
