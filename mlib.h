#ifndef MLIB_H
#define MLIB_H

//
// Transport
//
#define M_SOCK_PATH "maru-bridge"

//
// Opcodes
//
#define M_LOCK_BUFFER 'L'
#define M_UNLOCK_AND_POST_BUFFER 'U'

struct MDisplay {
    int sock_fd;        /* server socket */
    int buf_fd;         /* buffer handle */
};
typedef struct MDisplay MDisplay;

struct MBuffer {
    int width;
    int height;
    int stride;         /* may be >= width */
    void *bits;         /* raw buffer bytes in RGBA8888 format */
};
typedef struct MBuffer MBuffer;

int     MOpenDisplay    (MDisplay *dpy);
int     MCloseDisplay   (MDisplay *dpy);

int     MLockBuffer     (MDisplay *dpy, MBuffer *buf);
int     MUnlockBuffer   (MDisplay *dpy, MBuffer *buf);

#endif // MLIB_H