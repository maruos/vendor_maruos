#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include <binder/IBinder.h>
#include <ui/DisplayInfo.h>
#include <ui/Rect.h>
#include <gui/Surface.h>
#include <gui/ISurfaceComposer.h>
// #include <gui/ISurfaceComposerClient.h> createSurface() flags
#include <gui/SurfaceComposerClient.h>

#include <android/native_window.h> // ANativeWindow_Buffer full def

#include <cutils/log.h>
#include <utils/Errors.h>

#include "mlib.h"

#define DEBUG 0
#define BUF_SIZE (1 << 8)

using namespace android;

static const char *ACK = "K";

static void fillSurfaceRGBA8888(const sp<SurfaceControl>& sc,
        uint8_t r, uint8_t g, uint8_t b) {
    sp<Surface> s = sc->getSurface();

    ANativeWindow_Buffer outBuffer;
    buffer_handle_t handle;
    status_t err = s->lockWithHandle(&outBuffer, &handle, NULL);
    ALOGE_IF(err, "failed to lock buffer");

    // ALOGI("outBuffer dump");
    // ALOGI("outBuffer size: %d x %d", outBuffer.width, outBuffer.height);
    // ALOGI("outBuffer stride: %d", outBuffer.stride);

    // ALOGI("handle dump");
    // ALOGI("handle->version = %d", handle->version);
    // ALOGI("handle->numFds = %d", handle->numFds);
    // ALOGI("handle->numInts = %d", handle->numInts);
    // ALOGI("handle->data[0] = %x", handle->data[0]);


    uint8_t* buf = reinterpret_cast<uint8_t*>(outBuffer.bits);
    for (int32_t y = 0; y < outBuffer.height; ++y) {
        for (int32_t x = 0; x < outBuffer.width; ++x) {
            int32_t pixelBase = 4 * (y * outBuffer.stride + x);
            buf[pixelBase + 0] = r;
            buf[pixelBase + 1] = g;
            buf[pixelBase + 2] = b;
            buf[pixelBase + 3] = 100; // semi-transparent

            // ALOGI("(%d, %d) -> (%d, %d, %d, %d)", x, y, buf[pixelBase + 0], buf[pixelBase + 1], buf[pixelBase + 2], buf[pixelBase + 3]);
        }
    }

    err = s->unlockAndPost();
    ALOGE_IF(err, "failed to unlockAndPost() buffer");
}

/*
 * One can guesstimate fps based on the duration of a color
 * "pulse" created by this function as it resets the innermost
 * loop variable back to 0!
 *
 * I count ~4s pulses so we are rendering 256fp4s ~= 60fps!
 */
static void rainbowSurfaceRGBA8888(const sp<SurfaceControl>& sc) {
    for (int r = 0; r < 256; ++r) {
        for (int g = 0; g < 256; ++g) {
            for (int b = 0; b < 256; ++b) {
                fillSurfaceRGBA8888(sc, r, g, b);
            }
        }
    }
}

static int sendfd(const int sockfd, void *data, const int data_len, const int fd) {
    struct msghdr msg = {0}; // 0 initializer
    struct cmsghdr *cmsg;
    //int bufferFd = 1;
    union {
        char buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;
    int *fdptr;

    /* 
     * >= 1 byte of nonacillary data must be sent
     * in the same sendmsg() call to pass fds
     */
    struct iovec iov;
    // const char *ACK = "ACK";
    // iov.iov_base = (char *)ACK;
    // iov.iov_len = strlen(ACK);
    iov.iov_base = data;
    iov.iov_len = data_len;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    fdptr = (int *) CMSG_DATA(cmsg);
    memcpy(fdptr, &fd, sizeof(int));

    if (sendmsg(sockfd, &msg, 0) < 0) {
        ALOGE("Failed to sendmsg: %s", strerror(errno));
        return -1;
    }

    return 0;
}

static int lockBuffer(const sp<SurfaceControl>& sc, const int sockfd) {
    sp<Surface> s = sc->getSurface();

    ANativeWindow_Buffer outBuffer;
    buffer_handle_t handle;
    status_t err = s->lockWithHandle(&outBuffer, &handle, NULL);
    if (err != 0) {
        ALOGE("failed to lock buffer");
        return -1;
    }

    if (handle->numFds < 1) {
        ALOGE("buffer handle does not have any fds");
        return -1;
    }

    struct MBuffer mbuf;
    mbuf.width = outBuffer.width;
    mbuf.height = outBuffer.height;
    mbuf.stride = outBuffer.stride;
    mbuf.bits = NULL;

    return sendfd(sockfd, (void *)&mbuf, sizeof(mbuf), handle->data[0]);
}

static int unlockAndPostBuffer(const sp<SurfaceControl>& sc) {
    sp<Surface> s = sc->getSurface();

    return s->unlockAndPost();
}

int main() {

    ALOGD_IF(DEBUG, "Hello, World!!");

    //
    // Establish a connection with SurfaceFlinger
    //
    sp<SurfaceComposerClient> compositor = new SurfaceComposerClient;
    status_t check = compositor->initCheck();
    ALOGD_IF(DEBUG, "compositor->initCheck() = %d", check);
    if (NO_ERROR != check) {
        ALOGE("compositor->initCheck() failed!");
        return -1;
    }

    //
    // Create a Surface for rendering
    //
    DisplayInfo dinfo;
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain);
    SurfaceComposerClient::getDisplayInfo(display, &dinfo);

    ALOGI("DisplayInfo dump");
    ALOGI("     display w x h = %d x %d", dinfo.w, dinfo.h);
    ALOGI("     display orientation = %d", dinfo.orientation);

    uint32_t w = 1920;//dinfo.w;
    uint32_t h = 1080;//dinfo.h;

    sp<SurfaceControl> surfaceControl = compositor->createSurface(
                                            String8("Maru Surface"),
                                            w, h,
                                            PIXEL_FORMAT_BGRA_8888,
                                            0);
    if (surfaceControl == NULL || !surfaceControl->isValid()) {
        ALOGE("compositor->createSurface() failed!");
        return -1;
    }

    //
    // Display the surface on the screen
    //
    status_t ret = NO_ERROR;
    SurfaceComposerClient::openGlobalTransaction();
    ret |= surfaceControl->setLayer(0x7fffffff);
    ret |= surfaceControl->show();
    SurfaceComposerClient::closeGlobalTransaction(true);

    if (NO_ERROR != ret) {
        ALOGE("compositor transaction failed!");
    }
    
    //
    // Connect to bridge socket
    //
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ALOGE("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int len;
    struct sockaddr_un local, remote;

    local.sun_family = AF_UNIX;

    /* add a leading null byte to indicate abstract socket namespace */
    local.sun_path[0] = '\0';
    strcpy(local.sun_path + 1, M_SOCK_PATH);
    len = 1 + strlen(local.sun_path + 1) + sizeof(local.sun_family);
    
    /* unlink just in case...but abstract names should be auto destroyed */
    unlink(local.sun_path);
    int err = bind(sockfd, (struct sockaddr *)&local, len);
    if (err < 0) {
        ALOGE("Failed to bind socket: %s", strerror(errno));
        return -1;
    }

    err = listen(sockfd, 1);
    if (err < 0) {
        ALOGE("Failed to listen on socket: %s", strerror(errno));
    }

    ALOGD_IF(DEBUG, "Listening for client requests...");

    int cfd, t;
    t = sizeof(remote);
    cfd = accept(sockfd, (struct sockaddr *)&remote, &t);
    if (cfd < 0) {
        ALOGE("Failed to accept client: %s", strerror(errno));
    }

    do {
        int n;
        char buf[1];
        n = read(cfd, buf, 1);

        if (n < 0) {
            ALOGE("Failed to read from socket: %s", strerror(errno));
        }

        if (n == 0) {
            ALOGE("Client closed connection.");
            close(cfd);
            break;
        }

        ALOGD_IF(DEBUG, "n: %d", n);
        ALOGD_IF(DEBUG, "buf[0]: %c", buf[0]);
        switch (buf[0]) {
            case M_LOCK_BUFFER:
                ALOGD_IF(DEBUG, "Lock buffer request!");
                lockBuffer(surfaceControl, cfd);
                break;

            case M_UNLOCK_AND_POST_BUFFER:
                ALOGD_IF(DEBUG, "Unlock and post buffer request!");
                unlockAndPostBuffer(surfaceControl);
                break;

            default:
                ALOGW("Unrecognized request");
                /*
                 * WATCH OUT! Using write() AND sendmsg() at the
                 * same time to send a reply can result in mixed up
                 * order on the client-side when calling recvmsg() 
                 * and parsing the main data buffer.
                 * Basically, don't mix calls to write() and writev().
                 */
                if (write(cfd, ACK, strlen(ACK) + 1) < 0) {
                    ALOGE("Failed to write socket ACK: %s", strerror(errno));
                }
                break;
        }
    } while (1);


    //
    // Render test!
    //
    // rainbowSurfaceRGBA8888(surfaceControl);
    // fillSurfaceRGBA8888(surfaceControl, 0, 255, 0);

    display = NULL;
    surfaceControl = NULL;
    compositor = NULL;
    close(sockfd);
    return 0;
}
