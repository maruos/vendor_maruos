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
#include <X11/extensions/Xdamage.h>

#include "mlib.h"

#define BUF_SIZE (1 << 8)

/* TODO cursor cache */
static struct {
    uint32_t last_x;
    uint32_t last_y;
    uint32_t w;
    uint32_t h;
    uint8_t *bits;
} cursor_cache;

int copy_ximage_rows_to_buffer(MBuffer *buf, XImage *ximg,
         uint32_t row_start, uint32_t row_end) {
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
    for (y = row_start; y < row_end; ++y) {
        buf_row = buf->bits + (y * buf_bytes_per_line);
        ximg_row = ximg->data + (y * ximg->bytes_per_line);

        /*
         * we don't want to copy any extra XImage row padding
         * so we just copy up to image width instead of bytes_per_line
         */
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

int copy_ximage_to_buffer(MBuffer *buf, XImage *ximg) {
    return copy_ximage_rows_to_buffer(buf, ximg, 0, ximg->height);
}

int copy_xcursor_to_buffer(MBuffer *buf, XFixesCursorImage *cursor) {
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
                break;
            }

            uint8_t *pixel = cursor_cache.bits +
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

int Xrender(Display *dpy, MBuffer *buf, XImage *ximg, char repaint) {
    /* grab the current root window framebuffer */
    //printf("[DEBUG] grabbing root window...\n");
    if (repaint) {
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
    }

    XFixesCursorImage *cursor = XFixesGetCursorImage(dpy);
    //printf("[DEBUG] cursor pos: (%d, %d)\n", cursor->x, cursor->y);
    //printf("[DEBUG] w = %d, h = %d\n", cursor->width, cursor->height);
    //printf("[DEBUG] xhot = %d, yhot = %d\n", cursor->xhot, cursor->yhot);

    if (cursor_cache.bits == NULL) {
        cursor_cache.bits = (uint8_t *)cursor->pixels;
    }

    copy_ximage_rows_to_buffer(buf, ximg,
         cursor_cache.last_y, cursor_cache.last_y + cursor_cache.h);

    cursor_cache.last_x = cursor->x;
    cursor_cache.last_y = cursor->y;
    cursor_cache.w = cursor->width;
    cursor_cache.h = cursor->height;

    copy_xcursor_to_buffer(buf, cursor);

    return 0;
 }

int run(Display *dpy, MDisplay *mdpy, XImage *ximg, char repaint) {
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

    Xrender(dpy, &buf, ximg, repaint);
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
    int xfixes_event_base, error;
    if (!XFixesQueryExtension(dpy, &xfixes_event_base, &error)) {
        fprintf(stderr, "Xfixes extension unavailable!\n");
        return -1;
    }

    if (!XShmQueryExtension(dpy)) {
        fprintf(stderr, "XShm extension unavailable!\n");
        return -1;
    }

    int xdamage_event_base;
    if (!XDamageQueryExtension(dpy, &xdamage_event_base, &error)) {
        fprintf(stderr, "XDamage extension unavailable!\n");
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

    // unsigned long background, border;
    // background = BlackPixel(dpy, screen);
    // border = WhitePixel(dpy, screen);

    // int width = XDisplayWidth(dpy, screen);
    // int height = XDisplayHeight(dpy, screen);

    // Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
    //                             0, 0,
    //                             width, height,
    //                             2, border,
    //                             background);


    //
    // register for X events
    //
    XFixesSelectCursorInput(dpy, DefaultRootWindow(dpy),
         XFixesDisplayCursorNotifyMask);
    XSelectInput(dpy, DefaultRootWindow(dpy), PointerMotionMask);

    XDamageCreate(dpy, DefaultRootWindow(dpy), XDamageReportRawRectangles);


    // Window root, parent;
    // Window *children;
    // unsigned int num_children;
    // if (XQueryTree(dpy, DefaultRootWindow(dpy),
    //      &root, &parent, &children, &num_children)) {
    //     fprintf(stderr, "num_children: %d\n", num_children);
    //     if (children != NULL) {
    //         int i;
    //         for (i = 0; i < num_children; ++i) {
    //             fprintf(stderr, "[DEBUG] selecting input for child %d\n", i);
    //             XSelectInput(dpy, children[i], PointerMotionMask);
    //         }
    //         XFree(children);
    //     }
    // } else {
    //     fprintf(stderr, "error calling XQueryTree\n");
    // }
    // XGrabPointer(dpy, DefaultRootWindow(dpy), True, 
    //     PointerMotionMask, GrabModeAsync, GrabModeAsync, None,
    //     None, CurrentTime);

    // XMapWindow(dpy, win);

    /* event loop */
    XEvent ev;
    Bool repaint;
    for (;;) {
        repaint = XCheckTypedEvent(dpy, xdamage_event_base + XDamageNotify, &ev);
        run(dpy, &mdpy, ximg, repaint);
        // XNextEvent(dpy, &ev);
        // if (ev.type == xdamage_event_base + XDamageNotify) {
        //     fprintf(stderr, "XDamageNotify!\n");
        //     run(dpy, &mdpy, ximg, 1);
        // } else if (ev.type == MotionNotify) {
        //     fprintf(stderr, "MotionNotify!\n");
        //     run(dpy, &mdpy, ximg);
        // } else if (ev.type == xfixes_event_base + XFixesDisplayCursorNotify) {
        //     fprintf(stderr, "XFixesDisplayCursorNotify!\n");
        // }
        // switch (ev.type) {
        // // case XFixesDisplayCursorNotify:
        // //     fprintf(stderr, "XFixesDisplayCursorNotify event!\n");
        // //     // cev = (XFixesCursorNotifyEvent)ev;
        // //     // fprintf(stderr, "cursor-serial: %d\n", cev.cursor_serial);
        // //     break;

        // case MotionNotify:
        //     fprintf(stderr, "pointer coords = (%d, %d)\n",
        //          ev.xmotion.x_root, ev.xmotion.y_root);
        //     break;

        // case xdamage_event_base + XDamageNotify:
        //     fprintf(stderr, "XDamageNotify!\n");
        //     run(dpy, &mdpy, ximg);
        //     break;

        // default:
        //     fprintf(stderr, "received unexpected event: %d\n", ev.type);
        //     break;
        // }
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