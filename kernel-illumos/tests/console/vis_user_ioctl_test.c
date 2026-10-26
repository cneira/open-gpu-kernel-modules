/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Run on an illumos host where /dev/fb resolves to NVIDIA's boot GPU.
 * A NULL request is harmless, but reaches gfxp_fb's copyin without the
 * NVIDIA ioctl gate. It must be rejected before any console drawing code.
 */
#include <sys/visual_io.h>
#include <sys/kd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* visual_io.h intentionally hides these console-only commands from userland. */
#define VIS_CONSCLEAR_USER   (('V' << 8) | 8)
#define VIS_CONSCOPY_USER    (('V' << 8) | 7)
#define VIS_CONSDISPLAY_USER (('V' << 8) | 5)
#define VIS_CONSCURSOR_USER  (('V' << 8) | 3)

static int
check_user_ioctl(int fd, int cmd, intptr_t arg, const char *name)
{
    int rc;

    errno = 0;
    rc = ioctl(fd, cmd, arg);
    if (rc == -1 && errno == ENOTTY)
        return 0;

    fprintf(stderr, "%s: expected ENOTTY, got rc=%d errno=%d (%s)\n",
        name, rc, errno, strerror(errno));
    return 1;
}

int
main(void)
{
    struct vis_identifier ident = { 0 };
    int failures = 0;
    int fd;

    fd = open("/dev/fb", O_RDWR);
    if (fd == -1) {
        perror("open /dev/fb");
        return 1;
    }
    if (ioctl(fd, VIS_GETIDENTIFIER, &ident) == -1 ||
        strcmp(ident.name, "illumos_fb") != 0) {
        fprintf(stderr, "NVIDIA framebuffer console not active\n");
        (void) close(fd);
        return 1;
    }

    failures += check_user_ioctl(fd, VIS_CONSCLEAR_USER, 0,
        "VIS_CONSCLEAR");
    failures += check_user_ioctl(fd, VIS_CONSCOPY_USER, 0,
        "VIS_CONSCOPY");
    failures += check_user_ioctl(fd, VIS_CONSDISPLAY_USER, 0,
        "VIS_CONSDISPLAY");
    failures += check_user_ioctl(fd, VIS_CONSCURSOR_USER, 0,
        "VIS_CONSCURSOR");
    /* An invalid mode is rejected without touching the display. */
    failures += check_user_ioctl(fd, KDSETMODE, 12345, "KDSETMODE");

    (void) close(fd);
    return failures == 0 ? 0 : 1;
}
