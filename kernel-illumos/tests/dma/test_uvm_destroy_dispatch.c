/* Tracked OS imports use checked destruction; ordinary maps keep legacy path. */
#include <assert.h>
#include <stddef.h>

typedef int NV_STATUS;
#define NV_OK 0
#define NV_ERR_OPERATING_SYSTEM 1

struct list_head { int unused; };
typedef struct { void *import_token; } uvm_ext_gpu_mem_handle;
typedef struct { uvm_ext_gpu_mem_handle *mem_handle; int present; } uvm_ext_gpu_map_t;
typedef struct { int unused; } uvm_va_range_external_t;

static NV_STATUS checked_status;
static int checked_calls;
static int legacy_calls;

static NV_STATUS
uvm_ext_gpu_map_destroy_tracked(uvm_va_range_external_t *range,
                                uvm_ext_gpu_map_t *map)
{
    (void)range;
    ++checked_calls;
    if (checked_status == NV_OK)
        map->present = 0;
    return checked_status;
}

static void
uvm_ext_gpu_map_destroy_legacy(uvm_va_range_external_t *range,
                               uvm_ext_gpu_map_t *map,
                               struct list_head *deferred)
{
    (void)range;
    (void)deferred;
    ++legacy_calls;
    if (map != NULL)
        map->present = 0;
}

#include "../../../kernel-open/nvidia-uvm/uvm_map_external_destroy_dispatch_impl.h"

int main(void)
{
    uvm_va_range_external_t range = {0};
    uvm_ext_gpu_mem_handle tracked_handle = {(void *)1};
    uvm_ext_gpu_mem_handle ordinary_handle = {NULL};
    uvm_ext_gpu_map_t tracked = {&tracked_handle, 1};
    uvm_ext_gpu_map_t ordinary = {&ordinary_handle, 1};

    checked_status = NV_ERR_OPERATING_SYSTEM;
    assert(uvm_ext_gpu_map_destroy(&range, &tracked, NULL) ==
           NV_ERR_OPERATING_SYSTEM);
    assert(tracked.present && checked_calls == 1 && legacy_calls == 0);

    checked_status = NV_OK;
    assert(uvm_ext_gpu_map_destroy(&range, &tracked, NULL) == NV_OK);
    assert(!tracked.present && checked_calls == 2 && legacy_calls == 0);

    assert(uvm_ext_gpu_map_destroy(&range, &ordinary, NULL) == NV_OK);
    assert(!ordinary.present && checked_calls == 2 && legacy_calls == 1);
    assert(uvm_ext_gpu_map_destroy(&range, NULL, NULL) == NV_OK);
    assert(legacy_calls == 2);
    return 0;
}
