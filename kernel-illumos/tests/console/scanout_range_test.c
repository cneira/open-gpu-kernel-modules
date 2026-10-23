#include <stdint.h>
#include <stdio.h>

#include "../../nvidia/nv-console-range.h"

int
main(void)
{
    static const struct {
        const char *name;
        uint64_t bar, bar_size, scanout, scanout_size, offset;
        int exact, expected;
    } cases[] = {
        { "fb subrange", 0x1000, 0x1000, 0x1800, 0x400, 0, 0, 1 },
        { "imem offset", 0x20000000, 0x2000000, 0x21000000,
            0x400000, 0x1000000, 1, 1 },
        { "zero bar", 0x1000, 0, 0x1000, 1, 0, 0, 0 },
        { "zero scanout", 0x1000, 0x1000, 0x1000, 0, 0, 0, 0 },
        { "small imem", 0x20000000, 0xffffff, 0x21000000,
            1, 0x1000000, 1, 0 },
        { "fb end", 0x1000, 0x1000, 0x1fff, 2, 0, 0, 0 },
        { "imem end", 0x20000000, 0x1100000, 0x21000000,
            0x100001, 0x1000000, 1, 0 },
        { "wrong imem start", 0x20000000, 0x2000000, 0x21001000,
            0x1000, 0x1000000, 1, 0 },
        { "bar overflow", UINT64_MAX - 1, 4, UINT64_MAX - 1,
            1, 0, 0, 0 },
        { "scanout overflow", UINT64_MAX - 0x100, 0x101,
            UINT64_MAX - 0x10, 0x20, 0, 0, 0 },
        { "below bar", 0x1000, 0x1000, 0xfff, 1, 0, 0, 0 },
    };
    unsigned int i;

    for (i = 0; i < sizeof (cases) / sizeof (cases[0]); i++) {
        int got = nv_console_range_fits(cases[i].bar, cases[i].bar_size,
            cases[i].scanout, cases[i].scanout_size, cases[i].offset,
            cases[i].exact);
        if (got != cases[i].expected) {
            fprintf(stderr, "%s: got %d, expected %d\n", cases[i].name,
                got, cases[i].expected);
            return (1);
        }
    }
    return (0);
}
