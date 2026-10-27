#include <assert.h>
#include <stdio.h>

#include "../../../src/nvidia/inc/kernel/rmapi/nv_gpu_ops_original_gate.h"

static nv_original_gate_input_t
safe_original(void)
{
    nv_original_gate_input_t input = {
        .dup_count = 1,
        .ref_count = 2,
        .guards = 1,
        .all_imports_complete = 1,
        .cpu_mapping_count = 1,
        .cpu_mapping_is_os_descriptor = 1,
    };
    return input;
}

int
main(void)
{
    nv_original_gate_input_t input = safe_original();

    assert(nv_original_gate_allows_free(&input));
    input.ref_count = 3;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.dup_count = 2;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.all_imports_complete = 0;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.inter_mapping_count = 1;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.cpu_mapping_is_os_descriptor = 0;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.backref_count = 1;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.inter_backref_count = 1;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.child_count = 1;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.dependent_count = 1;
    assert(!nv_original_gate_allows_free(&input));
    input = safe_original();
    input.guards = 0;
    input.ref_count = 1;
    assert(nv_original_gate_allows_free(&input));

    assert(nv_original_guard_action(1, 1, 1) ==
           NV_ORIGINAL_GUARD_TRANSFER);
    assert(nv_original_guard_action(1, 0, 0) ==
           NV_ORIGINAL_GUARD_TRANSFER);
    assert(nv_original_guard_action(1, 0, 1) ==
           NV_ORIGINAL_GUARD_RELEASE);
    assert(nv_original_guard_action(0, 0, 1) ==
           NV_ORIGINAL_GUARD_NONE);

    puts("original OS descriptor gate: 15 cases passed");
    return 0;
}
