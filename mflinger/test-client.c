#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include "mlib.h"

#define BUF_SIZE (1 << 8)

static void fillBufferRGBA8888(uint8_t *buf,
         const uint32_t l, const uint32_t t,
         const uint32_t w, const uint32_t h, const uint32_t stride,
         const uint8_t r, const uint8_t g, const uint8_t b) {
    uint32_t x, y;
    for (y = t; y < t+h; ++y) {
        for (x = l; x < l+w; ++x) {
            uint32_t pixelBase = 4 * (y * stride + x);
            buf[pixelBase + 0] = r;
            buf[pixelBase + 1] = g;
            buf[pixelBase + 2] = b;
            buf[pixelBase + 3] = 255; // opaque

            // printf("(%d, %d) -> (%d, %d, %d, %d)", x, y, buf[pixelBase + 0], buf[pixelBase + 1], buf[pixelBase + 2], buf[pixelBase + 3]);
        }
    }
}

static void bounce(MDisplay *dpy, MBuffer *buf,
        int l, int t, int w, int h) {
    static int32_t xpos;
    static int32_t ypos;
    static int32_t dx;
    static int32_t dy;

    if (dx == 0) { dx = 6; }
    if (dy == 0) { dy = 3; }
    if (xpos == 0) { xpos = l + 10; }
    if (ypos == 0) { ypos = t + 100; }

    xpos += dx;
    ypos += dy;

    if (l > xpos||xpos > l+w) { dx = -dx; }
    if (t > ypos||ypos > t+h) { dy = -dy; }

    // printf("[DEBUG] xpos = %d\n", xpos);
    // printf("[DEBUG] ypos = %d\n", ypos);
    // printf("[DEBUG] dx = %d\n", dx);
    // printf("[DEBUG] dy = %d\n", dy);

    // fillBufferRGBA8888(buf, xpos, ypos, 16, 16, stride, 0, 255, 0);

    if (MUpdateBuffer(dpy, buf, xpos, ypos) < 0) {
        printf("[bounce] Error calling MUpdateBuffer\n");
    }
}

int run(MDisplay *dpy, MBuffer *buf, 
        uint8_t r, uint8_t g, uint8_t b) {
    int err;

    err = MLockBuffer(dpy, buf);
    if (err < 0) {
        printf("MLockBuffer failed: %s\n", strerror(errno));
        return -1;
    }

    printf("[DEBUG] buf.width = %d\n", buf->width);
    printf("[DEBUG] buf.height = %d\n", buf->height);
    // printf("[DEBUG] buf.stride = %d\n", buf.stride);
    // printf("[DEBUG] buf_fd = %d\n", buf_fd);

    fillBufferRGBA8888((uint8_t *)buf->bits, 0, 0,
         buf->width, buf->height, buf->stride, r, g, b);
    // bounce((uint8_t *)vaddr, 10, 10, 1070, 1910);

    err = MUnlockBuffer(dpy, buf);
    if (err < 0) {
        printf("MUnlockBuffer failed!\n");
        return -1;
    }

    return 0;
}

/*
 * One can guesstimate fps based on the duration of a color
 * "pulse" created by this function!
 *
 * I count ~4s pulses so we are rendering 256fp4s ~= 60fps!
 */
static void pulseBufferTest(MDisplay *dpy, MBuffer *buf, int reps) {
    int i, g;
    for (i = 0; i < reps; ++i) {
        printf("iteration %d\n", i);
        for (g = 0; g < 256; ++g) {
            run(dpy, buf, 0, g, 0);
        }
        for (g = 255; g >= 0; --g) {
            run(dpy, buf, 0, g, 0);
        }
    }
}

static void spriteTest(MDisplay *dpy, MBuffer *buf, int steps) {
    int i;
    for (i = 0; i < steps; ++i) {
        bounce(dpy, buf, 10, 10, 1070, 1910);
    }
}

int main(void) {
    int t, i, err;
    char buf[BUF_SIZE];

    MDisplay mdpy;
    if (MOpenDisplay(&mdpy) < 0) {
        printf("Error calling MOpenDisplay\n");
    }

    printf("Connected.\n");

    MBuffer root;
    root.width = 1080;
    root.height = 1920;
    if (MCreateBuffer(&mdpy, &root) < 0) {
        printf("Error calling MCreateBuffer\n");
        goto out;
    }
    printf("[DEBUG] root.__id = %d\n", root.__id);

    MBuffer pointer;
    pointer.width = 24;
    pointer.height = 24;
    if (MCreateBuffer(&mdpy, &pointer) < 0) {
        printf("Error calling MCreateBuffer\n");
        goto out;
    }
    printf("[DEBUG] pointer.__id = %d\n", pointer.__id);


    // paint pointer green
    run(&mdpy, &pointer, 0, 255, 0);
    spriteTest(&mdpy, &pointer, 500);

    pulseBufferTest(&mdpy, &root, 1);
    pulseBufferTest(&mdpy, &pointer, 1);

    if (MUpdateBuffer(&mdpy, &pointer, 500, 500) < 0) {
        printf("Error calling MUpdateBuffer\n");
    }

    pulseBufferTest(&mdpy, &pointer, 1);

    while(printf("> "), fgets(buf, BUF_SIZE, stdin), !feof(stdin)) {

        /* strip the newline */
        for (i = 0; i < BUF_SIZE; ++i) {
            if (buf[i] == '\n') {
                buf[i] = '\0';
                break;
            }
        }

        /* str(len) + 1 to write the null terminator! */
        if (write(mdpy.sock_fd, buf, strlen(buf) + 1) == -1) {
            perror("write");
            return 1;
        }

        /* buffer test reponses */
        if (strlen(buf) == 1 && buf[0] == '0') {
            switch (buf[0]) {
                case '0': ; // empty statement for C label quirk
                    // printf("[DEBUG] In lockBuffer case\n");
                    // lockBuffer(s);
                    break;

                case '1':
                    break;
            }
        } else {
            if ((t = read(mdpy.sock_fd, buf, BUF_SIZE)) > 0) {
                printf("server> %s\n", buf);
            } else {
                if (t < 0) {
                    perror("read");
                } else {
                    printf("Server closed connection.\n");
                    return 1;
                }
            }
        }

    }

out:
    MCloseDisplay(&mdpy);
    return 0;
}
