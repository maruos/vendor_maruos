#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XShm.h>

#include "mlib.h"

#define BUF_SIZE (1 << 8)

/* TODO cursor cache */
static uint8_t *cursor_buf;

int copy_ximage_to_buffer(MBuffer *buf, XImage *ximg) {
    /* sanity checks */
    // if (buf->width < ximg->width ||
    //     buf->height < ximg->height) {
    //     fprintf(stderr, "output (%dx%d) insufficient for input (%dx%d)\n",
    //          buf->width, buf->height, ximg->width, ximg->height);
    //     return -1;
    // }

    /* TODO ximg->xoffset? */
    uint32_t buf_bytes_per_line = buf->stride * 4;
    uint32_t ximg_bytes_per_pixel = ximg->bits_per_pixel / 8;
    uint32_t y;
    // printf("[DEBUG] bytes pp = %d\n", bytes_per_pixel);
    // printf("[DEBUG] copying over XImage to Android buffer...\n");

    /* row-by-row copy to adjust for differing strides */
    uint32_t *buf_row, *ximg_row;
    for (y = 0; y < ximg->height; ++y) {
        buf_row = buf->bits + (y * buf_bytes_per_line);
        ximg_row = ximg->data + (y * ximg->bytes_per_line);
        memcpy(buf_row, ximg_row, ximg->width * ximg_bytes_per_pixel);
        // uint32_t x;
        // for (x = 0; x < 100 /*w*/; ++x) {
        //    uint8_t *pixel = ximg->data +        /* base buffer ptr */
        //        (y * ximg->bytes_per_line) +     /* scan line index within buffer */
        //        (x * ximg_bytes_per_pixel);           /* pixel index within scan line */
        //    printf("[DEBUG] (%d, %d) -> (%d, %d, %d, %d)\n",
        //          x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
        // }
    }

    return 0;
}

int Xrender(Display *dpy, MBuffer *buf, XImage *ximg) {
    /* grab the current root window framebuffer */
    //printf("[DEBUG] grabbing root window...\n");
    Status status;
    status = XShmGetImage(dpy,
                DefaultRootWindow(dpy),
                ximg,
                0, 0,
                AllPlanes);
    if(!status) {
        fprintf(stderr, "error calling XShmGetImage\n");
    }

    copy_ximage_to_buffer(buf, ximg);

    XFixesCursorImage *cursor = XFixesGetCursorImage(dpy);
    //printf("[DEBUG] cursor pos: (%d, %d)\n", cursor->x, cursor->y);
    //printf("[DEBUG] w = %d, h = %d\n", cursor->width, cursor->height);
    //printf("[DEBUG] xhot = %d, yhot = %d\n", cursor->xhot, cursor->yhot);

    if (cursor_buf == NULL) {
        cursor_buf = (uint8_t *)cursor->pixels;
    }

    //printf("[DEBUG] painting cursor...\n");
/*    for (y = cursor->y; y < cursor->y + cursor->height; ++y) {
        memcpy(buf + (y * 1152 * 4) + (cursor->x * 4),
         cursor_buf + ((y - cursor->y) * cursor->width * 4),
         cursor->width * 4);
    }
*/
    uint32_t cur_x, cur_y;  /* cursor relative coords */
    uint32_t x, y;          /* root window coords */
    for (cur_y = 0; cur_y < cursor->height; ++cur_y) {
        for (cur_x = 0; cur_x < cursor->width; ++cur_x) {
            x = cursor->x + cur_x;
            y = cursor->y + cur_y;

            /* bounds check! */
            if (y >= buf->height || x >= buf->width) {
                /* TODO this can prob be a break */
                continue;
            }

            uint8_t *pixel = cursor_buf +
                            4 * (cur_y * cursor->width + cur_x);

            // printf("[DEBUG] (%d, %d) -> (%d, %d, %d, %d)\n",
            //     cursor->x + x, cursor->y + y, pixel[0], pixel[1], pixel[2], pixel[3]);

            /* copy only if opaque pixel */
            if (pixel[3] == 255) {
                uint32_t *buf_pixel = buf->bits + (y * buf->stride + x) * 4;
                memcpy(buf_pixel, pixel, 4);
            }
        }
    }
    

    return 0;
 }

int run(Display *dpy, MDisplay *mdpy, XImage *ximg) {
    int err;
    MBuffer buf;

    err = MLockBuffer(mdpy, &buf);
    if (err < 0) {
        fprintf(stderr, "MLockBuffer failed!\n");
        return -1;
    }

    // printf("[DEBUG] buf.width = %d\n", buf.width);
    // printf("[DEBUG] buf.height = %d\n", buf.height);
    // printf("[DEBUG] buf.stride = %d\n", buf.stride);
    // printf("[DEBUG] buf_fd = %d\n", buf_fd);

    //printf("[DEBUG] XDisplayWidth = %d\n", XDisplayWidth(dpy, 0));
    //printf("[DEBUG] XDisplayHeight = %d\n", XDisplayHeight(dpy, 0));

    Xrender(dpy, &buf, ximg);
    // fillBufferRGBA8888((uint8_t *)buf.bits, 0, 0, buf.width, buf.height, r, g, b);

    err = MUnlockBuffer(mdpy, &buf);
    if (err < 0) {
        fprintf(stderr, "MUnlockBuffer failed!\n");
        return -1;
    }

    return 0;
}

int main(void) {
    Display *dpy;
    MDisplay mdpy;
    int err = 0;

    /* TODO Ctrl-C handler to cleanup shm */

    /* connect to the X server using the DISPLAY environment variable */
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "error calling XOpenDisplay\n");
        return -1;
    }

    /* check for necessary eXtensions */
    int event, error;
    if (!XFixesQueryExtension(dpy, &event, &error)) {
        fprintf(stderr, "Xfixes extension unavailable!\n");
        return -1;
    }

    if (!XShmQueryExtension(dpy)) {
        fprintf(stderr, "XShm extension unavailable!\n");
        return -1;
    }

    /* connect to maru display server */
    if (MOpenDisplay(&mdpy) < 0) {
        fprintf(stderr, "error calling MOpenDisplay\n");
        return -1;
    }

    //
    // create shared memory XImage structure
    //
    XShmSegmentInfo shminfo;
    int screen = DefaultScreen(dpy);
    XImage *ximg = XShmCreateImage(dpy,
                        DefaultVisual(dpy, screen),
                        DefaultDepth(dpy, screen),
                        ZPixmap,
                        NULL,
                        &shminfo,
                        XDisplayWidth(dpy, screen),
                        XDisplayHeight(dpy, screen));
    if (ximg == NULL) {
        fprintf(stderr, "error creating XShm Ximage\n");
        return -1;
    }

    //
    // create a shared memory segment to store actual image data
    //
    shminfo.shmid = shmget(IPC_PRIVATE,
             ximg->bytes_per_line * ximg->height, IPC_CREAT|0777);
    if (shminfo.shmid < 0) {
        fprintf(stderr, "error creating shm segment: %s\n", strerror(errno));
        return -1;
    }

    shminfo.shmaddr = ximg->data = shmat(shminfo.shmid, NULL, 0);
    if (shminfo.shmaddr < 0) {
        fprintf(stderr, "error attaching shm segment: %s\n", strerror(errno));
        err = -1;
        goto cleanup_shm;
    }

    shminfo.readOnly = False;

    //
    // inform server of shm
    //
    if (!XShmAttach(dpy, &shminfo)) {
        fprintf(stderr, "error calling XShmAttach\n");
        err = -1;
        goto cleanup_X;
    }

    for (;;) {
        run(dpy, &mdpy, ximg);
    }

cleanup_X:

    if (!XShmDetach(dpy, &shminfo)) {
        fprintf(stderr, "error detaching shm from X server\n");
    }

    XDestroyImage(ximg);

cleanup_shm:

    if (shmdt(shminfo.shmaddr) < 0) {
        fprintf(stderr, "error detaching shm: %s\n", strerror(errno));
    }

    if (shmctl(shminfo.shmid, IPC_RMID, 0) < 0) {
        fprintf(stderr, "error destroying shm: %s\n", strerror(errno));
    }

    return err;
}