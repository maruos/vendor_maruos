#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>

#include <linux/input.h>

#include "mlib.h"
#include "mcursor_cache.h"

#define BUF_SIZE (1 << 8)

int copy_ximg_rows_to_buffer_mlocked(MBuffer *buf, XImage *ximg,
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
        ximg_row = (void *)ximg->data + (y * ximg->bytes_per_line);

        /*
         * we don't want to copy any extra XImage row padding
         * so we just copy up to image width instead of bytes_per_line
         */
        memcpy(buf_row, ximg_row, ximg->width * ximg_bytes_per_pixel);
    }

    return 0;
}

int copy_ximg_to_buffer_mlocked(MBuffer *buf, XImage *ximg) {
    return copy_ximg_rows_to_buffer_mlocked(buf, ximg, 0, ximg->height);
}

int copy_xcursor_to_buffer(MDisplay *mdpy, MBuffer *buf, XFixesCursorImage *cursor) {
    //printf("[DEBUG] painting cursor...\n");
/*    for (y = cursor->y; y < cursor->y + cursor->height; ++y) {
        memcpy(buf + (y * 1152 * 4) + (cursor->x * 4),
         cursor_buf + ((y - cursor->y) * cursor->width * 4),
         cursor->width * 4);
    }
*/

    // fprintf(stderr, "[DEBUG] cursor dims = %d x %d\n",
    //      cursor->width, cursor->height);

    int err;
    err = MLockBuffer(mdpy, buf);
    if (err < 0) {
        fprintf(stderr, "MLockBuffer failed!\n");
        return -1;
    }

    /* clear out stale pixels */
    memset(buf->bits, 0, buf->height * buf->stride * 4);

    uint32_t cur_x, cur_y;  /* cursor relative coords */
    uint32_t x, y;          /* root window coords */
    for (cur_y = 0; cur_y < cursor->height; ++cur_y) {
        for (cur_x = 0; cur_x < cursor->width; ++cur_x) {
            x = cur_x;
            y = cur_y;

            /* bounds check! */
            if (y >= buf->height || x >= buf->width) {
                break;
            }

            uint8_t *pixel = (uint8_t *)cursor->pixels +
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

    err = MUnlockBuffer(mdpy, buf);
    if (err < 0) {
        fprintf(stderr, "MUnlockBuffer failed!\n");
        return -1;
    }

    return 0;
}

int render_root(Display *dpy, MDisplay *mdpy,
        MBuffer *buf, XImage *ximg) {
    int err;

    err = MLockBuffer(mdpy, buf);
    if (err < 0) {
        fprintf(stderr, "MLockBuffer failed!\n");
        return -1;
    }

    // printf("[DEBUG] buf.width = %d\n", buf.width);
    // printf("[DEBUG] buf.height = %d\n", buf.height);
    // printf("[DEBUG] buf.stride = %d\n", buf.stride);
    // printf("[DEBUG] buf_fd = %d\n", buf_fd);

    Status status;
    status = XShmGetImage(dpy,
        DefaultRootWindow(dpy),
        ximg,
        0, 0,
        AllPlanes);
    if(!status) {
        fprintf(stderr, "error calling XShmGetImage\n");
    }

    copy_ximg_to_buffer_mlocked(buf, ximg);

    // fillBufferRGBA8888((uint8_t *)buf.bits, 0, 0, buf.width, buf.height, r, g, b);

    err = MUnlockBuffer(mdpy, buf);
    if (err < 0) {
        fprintf(stderr, "MUnlockBuffer failed!\n");
        return -1;
    }

    return 0;
}

int update_cursor(Display *dpy, MDisplay *mdpy, MBuffer *cursor) {
    Window root, child;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask;
    XQueryPointer(dpy, DefaultRootWindow(dpy),
        &root, &child,
        &root_x, &root_y,
        &win_x, &win_y,
        &mask);

    // fprintf(stderr, "[DEBUG] cursor coords: (%d, %d)\n",
    //      root_x, root_y);

    int last_x, last_y;
    cursor_cache_get_last_pos(&last_x, &last_y);
    if (root_x != last_x || root_y != last_y) {
        XFixesCursorImage *xcursor = cursor_cache_get_cur();

        /* adjust so that hotspot is top-left */
        int32_t xpos = root_x - xcursor->xhot;
        int32_t ypos = root_y - xcursor->yhot;

        /* enforce lower bound or surfaceflinger freaks out */
        if (xpos < 0) {
            xpos = 0;
        }
        if (ypos < 0) {
            ypos = 0;
        }

        if (MUpdateBuffer(mdpy, cursor, xpos, ypos) < 0) {
            fprintf(stderr, "error calling MUpdateBuffer\n");
        }

        cursor_cache_set_last_pos(root_x, root_y);
    }

    return 0;
}

struct cursor_thread_args {
    MDisplay *mdpy;
    MBuffer *cursor;
};

void *cursor_thread(void *targs) {
    struct cursor_thread_args *args = (struct cursor_thread_args *)targs;

    /* separate threads need separate client connections */
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "[cursor_thread] error calling XOpenDisplay\n");
        return (void *)-1;
    }

    /* TODO handle pre-pairing */
    int cursor_fd = open("/dev/input/event6", O_RDONLY);
    if (cursor_fd < 0) {
        fprintf(stderr, "error opening cursor evdev file: %s\n",
            strerror(errno));
    }

    /*
     * Sadly, the X protocol is incapable of delivering
     * pointer motion events for the entire root window.
     * You can try to get around this by XSelectInput()
     * on all windows returned by XQueryTree() but root
     * window motion is still not included. XGrabPointer()
     * will deliver events but blocks all other clients,
     * i.e. none of your windows will respond to the cursor.
     *
     * What do we do? Either repeatedly poll (which sucks
     * majorly for eating the CPU), or take matters into our
     * own hands...
     *
     * Poll loop: we directly monitor the cursor evdev file
     * for new events as a trigger to update the pointer.
     */
    struct pollfd pollfds[1];
    struct input_event ev;
    int timeout = -1;
    int err;
    pollfds[0].fd = cursor_fd;
    pollfds[0].events = POLLIN;
    fprintf(stderr, "[DEBUG] polling cursor...\n");
    do {
        err = poll(pollfds, 1, timeout);

        if (err < 0) {
            fprintf(stderr, "polling error: %s\n", strerror(errno));
            continue;
        }

        if (pollfds[0].revents & POLLIN) {
            // fprintf(stderr, "[DEBUG] poll: cursor is ready to read!\n");

            /* we must read the file to clear the event queue */
            err = read(pollfds[0].fd, &ev, sizeof(ev));
            if (err < 0) {
                fprintf(stderr, "error calling read: %s\n", strerror(errno));
                continue;
            }
            // fprintf(stderr, "[DEBUG] read %d bytes\n", err);
            update_cursor(dpy, args->mdpy, args->cursor);
        }
    } while (1);

    XCloseDisplay(dpy);
    close(cursor_fd);
    return NULL;
}

int cleanup_shm(const void *shmaddr, const int shmid) {
    if (shmdt(shmaddr) < 0) {
        fprintf(stderr, "error detaching shm: %s\n", strerror(errno));
        return -1;
    }

    if (shmctl(shmid, IPC_RMID, 0) < 0) {
        fprintf(stderr, "error destroying shm: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

XImage *init_xshm(Display *dpy, XShmSegmentInfo *shminfo, int screen) {
    //
    // create shared memory XImage structure
    //
    XImage *ximg = XShmCreateImage(dpy,
                        DefaultVisual(dpy, screen),
                        DefaultDepth(dpy, screen),
                        ZPixmap,
                        NULL,
                        shminfo,
                        XDisplayWidth(dpy, screen),
                        XDisplayHeight(dpy, screen));
    if (ximg == NULL) {
        fprintf(stderr, "error creating XShm Ximage\n");
        return NULL;
    }

    //
    // create a shared memory segment to store actual image data
    //
    shminfo->shmid = shmget(IPC_PRIVATE,
             ximg->bytes_per_line * ximg->height, IPC_CREAT|0777);
    if (shminfo->shmid < 0) {
        fprintf(stderr, "error creating shm segment: %s\n", strerror(errno));
        return NULL;
    }

    shminfo->shmaddr = ximg->data = shmat(shminfo->shmid, NULL, 0);
    if (shminfo->shmaddr < 0) {
        fprintf(stderr, "error attaching shm segment: %s\n", strerror(errno));
        cleanup_shm(shminfo->shmaddr, shminfo->shmid);
        return NULL;
    }

    shminfo->readOnly = False;

    //
    // inform server of shm
    //
    if (!XShmAttach(dpy, shminfo)) {
        fprintf(stderr, "error calling XShmAttach\n");
        cleanup_shm(shminfo->shmaddr, shminfo->shmid);
        return NULL;
    }

    return ximg;
}

int main(void) {
    Display *dpy;
    MDisplay mdpy;
    int err = 0;

    /* TODO Ctrl-C handler to cleanup shm */

    /* must be first Xlib call for multi-threaded programs */
    if (!XInitThreads()) {
        fprintf(stderr, "error calling XInitThreads\n");
        return -1;
    }

    /* connect to the X server using the DISPLAY environment variable */
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "error calling XOpenDisplay\n");
        return -1;
    }

    //
    // check for necessary eXtensions
    //
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
    // Create necessary buffers
    //

    int screen = DefaultScreen(dpy);

    /* root window buffer */
    MBuffer root;
    root.width = XDisplayWidth(dpy, screen);
    root.height = XDisplayHeight(dpy, screen);
    if (MCreateBuffer(&mdpy, &root) < 0) {
        printf("Error calling MCreateBuffer\n");
    }
    printf("[DEBUG] root.__id = %d\n", root.__id);

    /* cursor buffer */
    XFixesCursorImage *xcursor = XFixesGetCursorImage(dpy);
    cursor_cache_add(xcursor);
    cursor_cache_set_cur(xcursor);

    MBuffer cursor;
    cursor.width = xcursor->width;
    cursor.height = xcursor->height;
    if (MCreateBuffer(&mdpy, &cursor) < 0) {
        printf("Error calling MCreateBuffer\n");
    }
    printf("[DEBUG] cursor.__id = %d\n", cursor.__id);

    /* render cursor sprite */
    if (copy_xcursor_to_buffer(&mdpy, &cursor, xcursor) < 0) {
        fprintf(stderr, "failed to render cursor sprite\n");
    }

    //
    // set up XShm
    //
    XShmSegmentInfo shminfo;
    XImage *ximg = init_xshm(dpy, &shminfo, screen);

    //
    // register for X events
    //

    /* let me know when the cursor image changes */
    XFixesSelectCursorInput(dpy, DefaultRootWindow(dpy),
        XFixesDisplayCursorNotifyMask);

    /* report a single damage event if the damage region is non-empty */
    Damage damage = XDamageCreate(dpy, DefaultRootWindow(dpy),
        XDamageReportNonEmpty);


    //
    // spawn cursor thread
    //
    pthread_t cursor_pthread;
    struct cursor_thread_args args;
    args.mdpy = &mdpy;
    args.cursor = &cursor;
    pthread_create(&cursor_pthread, NULL,
        &cursor_thread, (void *)&args);

    //
    // event loop
    //
    XEvent ev;
    do {
        XNextEvent(dpy, &ev);
        if (ev.type == xdamage_event_base + XDamageNotify) {
            XDamageNotifyEvent *dmg = (XDamageNotifyEvent *)&ev;
            // fprintf(stderr, "[DEBUG] dmg>more = %d\n", dmg->more);
            // fprintf(stderr, "[DEBUG] dmg->area pos (%d, %d)\n",
            //      dmg->area.x, dmg->area.y);
            // fprintf(stderr, "[DEBUG] dmg->area dims %dx%d\n", 
            //      dmg->area.width, dmg->area.height);
            /* TODO opt: only render damaged areas */
            render_root(dpy, &mdpy, &root, ximg);

            /* clear out all the damage (so we can get another event) */
            XDamageSubtract(dpy, dmg->damage, None, None);
        } else if (ev.type == xfixes_event_base + XFixesCursorNotify) {
            fprintf(stderr, "[DEBUG] XFixesCursorNotifyEvent!\n");
            XFixesCursorNotifyEvent *cev = (XFixesCursorNotifyEvent *)&ev;
            fprintf(stderr, "[DEBUG] cursor_serial: %lu\n", cev->cursor_serial);

            /* first, check if we have the new cursor in our cache... */
            XFixesCursorImage *xcursor = cursor_cache_get(cev->cursor_serial);

            /* ...if not, make the server request */
            if (xcursor == NULL) {
                xcursor = XFixesGetCursorImage(dpy);
                cursor_cache_add(xcursor);
            }

            /* render the new cursor */
            if (copy_xcursor_to_buffer(&mdpy, &cursor, xcursor) < 0) {
                fprintf(stderr, "failed to render cursor sprite\n");
            }

            cursor_cache_set_cur(xcursor);
        }
    } while (1);


    if (!XShmDetach(dpy, &shminfo)) {
        fprintf(stderr, "error detaching shm from X server\n");
    }
    cleanup_shm(shminfo.shmaddr, shminfo.shmid);

    cursor_cache_free();
    XDestroyImage(ximg);
    XDamageDestroy(dpy, damage);
    XCloseDisplay(dpy);

    return err;
}
