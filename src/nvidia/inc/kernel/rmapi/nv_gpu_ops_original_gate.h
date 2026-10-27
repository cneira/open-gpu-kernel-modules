/* SPDX-License-Identifier: MIT */
#ifndef _NV_GPU_OPS_ORIGINAL_GATE_H_
#define _NV_GPU_OPS_ORIGINAL_GATE_H_

/* A conservative preflight before RM frees a pinned OS descriptor. */
typedef struct nv_original_gate_input {
    unsigned int dup_count;
    unsigned int ref_count;
    unsigned int guards;
    unsigned int all_imports_complete;
    unsigned int cpu_mapping_count;
    unsigned int cpu_mapping_is_os_descriptor;
    unsigned int backref_count;
    unsigned int inter_mapping_count;
    unsigned int inter_backref_count;
    unsigned int child_count;
    unsigned int dependent_count;
} nv_original_gate_input_t;

typedef enum nv_original_guard_result {
    NV_ORIGINAL_GUARD_NONE,
    NV_ORIGINAL_GUARD_TRANSFER,
    NV_ORIGINAL_GUARD_RELEASE,
} nv_original_guard_result_t;

static inline nv_original_guard_result_t
nv_original_guard_action(unsigned int guards, unsigned int dup_count,
    unsigned int all_imports_complete)
{
    if (guards == 0)
        return NV_ORIGINAL_GUARD_NONE;
    if (dup_count != 0 || !all_imports_complete)
        return NV_ORIGINAL_GUARD_TRANSFER;
    return NV_ORIGINAL_GUARD_RELEASE;
}

static inline int
nv_original_gate_allows_free(const nv_original_gate_input_t *in)
{
    return in != 0 && in->dup_count == 1 &&
        in->ref_count == in->guards + 1 &&
        in->all_imports_complete &&
        in->cpu_mapping_count == 1 &&
        in->cpu_mapping_is_os_descriptor &&
        in->backref_count == 0 &&
        in->inter_mapping_count == 0 &&
        in->inter_backref_count == 0 &&
        in->child_count == 0 &&
        in->dependent_count == 0;
}

#endif
