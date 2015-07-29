#ifndef MLIB_H
#define MLIB_H

struct MDisplay {
    int sock_fd;        /* server socket */
};
typedef struct MDisplay MDisplay;

struct MBuffer {
    uint32_t width;
    uint32_t height;
    uint32_t stride;    /* may be >= width */
    void *bits;         /* raw buffer bytes in BGRA8888 format */

    int __fd;
    int32_t __id;
};
typedef struct MBuffer MBuffer;

int     MOpenDisplay    (MDisplay *dpy);
int     MCloseDisplay   (MDisplay *dpy);

//
// Buffer management
//
int     MCreateBuffer   (MDisplay *dpy, MBuffer *buf);
int     MUpdateBuffer   (MDisplay *dpy, MBuffer *buf,
                         uint32_t xpos, uint32_t ypos);

//
// Buffer rendering
//
int     MLockBuffer     (MDisplay *dpy, MBuffer *buf);
int     MUnlockBuffer   (MDisplay *dpy, MBuffer *buf);

#endif // MLIB_H