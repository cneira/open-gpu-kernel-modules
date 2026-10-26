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
