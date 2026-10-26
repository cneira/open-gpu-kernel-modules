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

/* Overflow-safe check that the boot scanout starts at bar_base + offset. */
#ifndef NV_CONSOLE_RANGE_H
#define NV_CONSOLE_RANGE_H

#include <sys/types.h>
#include <stdint.h>

static inline int
nv_console_range_fits(uint64_t bar_base, uint64_t bar_size,
    uint64_t scanout_base, uint64_t scanout_size, uint64_t offset)
{
    if (bar_base == 0 || bar_size == 0 || scanout_size == 0 ||
        bar_size - 1 > UINT64_MAX - bar_base ||
        offset >= bar_size ||
        scanout_base < bar_base || scanout_base - bar_base != offset)
        return (0);

    return (scanout_size <= bar_size - offset);
}

#endif
