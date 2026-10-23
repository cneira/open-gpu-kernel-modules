/* Overflow-safe containment of the boot scanout within one GPU BAR. */
#ifndef NV_CONSOLE_RANGE_H
#define NV_CONSOLE_RANGE_H

#include <sys/types.h>
#include <stdint.h>

static inline int
nv_console_range_fits(uint64_t bar_base, uint64_t bar_size,
    uint64_t scanout_base, uint64_t scanout_size, uint64_t required_offset,
    int exact_start)
{
    uint64_t start;

    if (bar_base == 0 || bar_size == 0 || scanout_size == 0 ||
        bar_size - 1 > UINT64_MAX - bar_base ||
        scanout_base < bar_base ||
        scanout_size - 1 > UINT64_MAX - scanout_base ||
        required_offset >= bar_size)
        return (0);

    start = scanout_base - bar_base;
    if (start < required_offset || (exact_start && start != required_offset))
        return (0);

    return (start < bar_size && scanout_size <= bar_size - start);
}

#endif
