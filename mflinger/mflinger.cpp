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
#include "mlib-protocol.h"

#define DEBUG (1)

#define BUF_SIZE (1 << 8)
#define MAX_SURFACES (2)

using namespace android;

struct mflinger_state {
    sp<SurfaceComposerClient> compositor;       /* SurfaceFlinger connection */
    sp<SurfaceControl> surfaces[MAX_SURFACES];  /* surfaces alloc'd for clients */
    int num_surfaces;                           /* number of surfaces currently managed */
};

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

static int32_t buffer_id_to_index(int32_t id) {
    return id - 1;
}

static int createSurface(struct mflinger_state *state,
            uint32_t w, uint32_t h) {

    if (state->num_surfaces >= MAX_SURFACES) {
        return -1;
    }

    String8 name = String8::format("maru %d", state->num_surfaces);
    sp<SurfaceControl> surface = state->compositor->createSurface(
                                name,
                                w, h,
                                PIXEL_FORMAT_BGRA_8888,
                                0);
    if (surface == NULL || !surface->isValid()) {
        ALOGE("compositor->createSurface() failed!");
        return -1;
    }

    //
    // Display the surface on the screen
    //
    status_t ret = NO_ERROR;
    SurfaceComposerClient::openGlobalTransaction();
    ret |= surface->setLayer(0x7ffffff0 + state->num_surfaces);
    ret |= surface->show();
    SurfaceComposerClient::closeGlobalTransaction(true);

    if (NO_ERROR != ret) {
        ALOGE("compositor transaction failed!");
        return -1;
    }

    state->surfaces[(state->num_surfaces)++] = surface;

    return 0;
}

static int createBuffer(const int sockfd, struct mflinger_state *state) {
    int n;
    MCreateBufferRequest request;
    n = read(sockfd, &request, sizeof(request));
    ALOGD_IF(DEBUG, "[C] n: %d", n);
    ALOGD_IF(DEBUG, "[C] requested dims = (%lux%lu)", 
        (unsigned long)request.width, (unsigned long)request.height);

    ALOGD_IF(DEBUG, "[C] 1 -- num_surfaces = %d", state->num_surfaces);

    n = createSurface(state,
         request.width, request.height);

    ALOGD_IF(DEBUG, "[C] 2 -- num_surfaces = %d", state->num_surfaces);

    MCreateBufferResponse response;
    response.id = n ? -1 : state->num_surfaces;
    response.result = n ? -1 : 0;

    if (write(sockfd, &response, sizeof(response)) < 0) {
        ALOGE("[C] Failed to write response: %s", strerror(errno));
        return -1;
    }

    return 0;
}

static int updateBuffer(const int sockfd, struct mflinger_state *state) {
    int n;
    MUpdateBufferRequest request;
    n = read(sockfd, &request, sizeof(request));
    ALOGD_IF(DEBUG, "[updateBuffer] n: %d", n);
    ALOGD_IF(DEBUG, "[updateBuffer] requested id = %d", request.id);
    ALOGD_IF(DEBUG, "[updateBuffer] requested pos = (%d, %d)",
        request.xpos, request.ypos);

    int32_t idx = buffer_id_to_index(request.id);

    if (0 <= idx && idx < state->num_surfaces) {
        sp<SurfaceControl> sc = state->surfaces[idx];

        status_t ret = NO_ERROR;
        SurfaceComposerClient::openGlobalTransaction();
        ret |= sc->setPosition(request.xpos, request.ypos);
        SurfaceComposerClient::closeGlobalTransaction();

        if (NO_ERROR != ret) {
            ALOGE("compositor transaction failed!");
            return -1;
        }

        return 0;
    }

    return -1;
}

static int sendfd(const int sockfd,
            void *data, const int data_len,
            const int fd) {
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

static int lockBuffer(const int sockfd, struct mflinger_state *state) {
    int n;
    MLockBufferRequest request;
    n = read(sockfd, &request, sizeof(request));
    ALOGD_IF(DEBUG, "[L] n: %d", n);
    ALOGD_IF(DEBUG, "[L] requested id = %d", request.id);
    int32_t idx = buffer_id_to_index(request.id);

    MLockBufferResponse response;
    response.result = -1;

    if (0 <= idx && idx < state->num_surfaces) {
        sp<SurfaceControl> sc = state->surfaces[idx];
        sp<Surface> s = sc->getSurface();

        ANativeWindow_Buffer outBuffer;
        buffer_handle_t handle;
        status_t err = s->lockWithHandle(&outBuffer, &handle, NULL);
        if (err != 0) {
            ALOGE("failed to lock buffer");
        } else if (handle->numFds < 1) {
            ALOGE("buffer handle does not have any fds");
        } else {
            /* all is well */
            response.buffer.width = outBuffer.width;
            response.buffer.height = outBuffer.height;
            response.buffer.stride = outBuffer.stride;
            response.buffer.bits = NULL;
            response.result = 0;

            return sendfd(sockfd, (void *)&response,
                sizeof(response), handle->data[0]);
        }
    } else {
        ALOGE("Invalid buffer id: %d\n", request.id);
    }    

    if (write(sockfd, &response, sizeof(response)) < 0) {
        ALOGE("[L] Failed to write response: %s", strerror(errno));
    }
    return -1;
}

static int unlockAndPostBuffer(const int sockfd,
            struct mflinger_state *state) {
    int n;
    MUnlockBufferRequest request;
    n = read(sockfd, &request, sizeof(request));
    ALOGD_IF(DEBUG, "[U] n: %d", n);
    ALOGD_IF(DEBUG, "[U] requested id = %d", request.id);
    int32_t idx = buffer_id_to_index(request.id);

    if (0 <= idx && idx < state->num_surfaces) {
        sp<SurfaceControl> sc = state->surfaces[idx];
        sp<Surface> s = sc->getSurface();

        return s->unlockAndPost();
    } else {
        ALOGE("Invalid buffer id: %d\n", request.id);
    }

    /* TODO return failure to client? */

    return -1;
}

static void purge_surfaces(struct mflinger_state *state) {
    do {
        /*
         * these are strong pointers so setting them
         * to NULL will trigger dtor()
         */
        state->surfaces[state->num_surfaces - 1] = NULL;
    } while (--state->num_surfaces > 0);
}

static void serve(const int sockfd, struct mflinger_state *state) {
    int cfd, t;
    struct sockaddr_un remote;

    ALOGD_IF(DEBUG, "Listening for client requests...");

    t = sizeof(remote);
    cfd = accept(sockfd, (struct sockaddr *)&remote, &t);
    if (cfd < 0) {
        ALOGE("Failed to accept client: %s", strerror(errno));
    }

    do {
        int n;
        uint32_t buf;
        n = read(cfd, &buf, sizeof(buf));

        if (n < 0) {
            ALOGE("Failed to read from socket: %s", strerror(errno));
        }

        if (n == 0) {
            ALOGE("Client closed connection.");
            close(cfd);
            purge_surfaces(state);
            break;
        }

        ALOGD_IF(DEBUG, "n: %d", n);
        ALOGD_IF(DEBUG, "buf: %d", buf);
        switch (buf) {
            case M_CREATE_BUFFER:
                ALOGD_IF(DEBUG, "Create buffer request!");
                createBuffer(cfd, state);
                break;

            case M_UPDATE_BUFFER:
                ALOGD_IF(DEBUG, "Update buffer request!");
                updateBuffer(cfd, state);
                break;

            case M_LOCK_BUFFER:
                ALOGD_IF(DEBUG, "Lock buffer request!");
                lockBuffer(cfd, state);
                break;

            case M_UNLOCK_AND_POST_BUFFER:
                ALOGD_IF(DEBUG, "Unlock and post buffer request!");
                unlockAndPostBuffer(cfd, state);
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
                // if (write(cfd, ACK, strlen(ACK) + 1) < 0) {
                //     ALOGE("Failed to write socket ACK: %s", strerror(errno));
                // }
                break;
        }
    } while (1);
}

int main() {

    struct mflinger_state state;

    //
    // Establish a connection with SurfaceFlinger
    //
    state.compositor = new SurfaceComposerClient;
    status_t check = state.compositor->initCheck();
    ALOGD_IF(DEBUG, "compositor->initCheck() = %d", check);
    if (NO_ERROR != check) {
        ALOGE("compositor->initCheck() failed!");
        return -1;
    }
    state.num_surfaces = 0;


    //
    // Display info
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

    //
    // Connect to bridge socket
    //
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ALOGE("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int len;
    struct sockaddr_un local;

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

    //
    // Serve loop
    //
    for (;;) {
        serve(sockfd, &state);
    }


    //
    // Render test!
    //
    // rainbowSurfaceRGBA8888(surfaceControl);
    // fillSurfaceRGBA8888(surfaceControl, 0, 255, 0);


    //
    // Cleanup
    //
    display = NULL;

    purge_surfaces(&state);
    state.compositor = NULL;

    close(sockfd);
    return 0;
}
