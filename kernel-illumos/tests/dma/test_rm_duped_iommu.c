/* UVM OS-descriptor copies must not be freed after failed IOMMU unmap. */
#include <assert.h>
#include <stddef.h>

typedef unsigned int NvU32;
typedef int NvBool;
typedef int NV_STATUS;
typedef struct memory_descriptor_s {
    int needs_iommu;
} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;
typedef struct memory_s {
    PMEMORY_DESCRIPTOR pMemDesc;
    int is_os_descriptor;
} Memory;
typedef struct os_desc_memory_s { int unused; } OsDescMemory;

#define NV_OK 0
#define NV_ERR_OPERATING_SYSTEM 2
#define NV_SUNOS 1
#define dynamicCast(memory, type) ((memory)->is_os_descriptor ? (type *)1 : NULL)

static NV_STATUS checked_result;
static int checked_count;
static int legacy_count;

static int
memdescRequiresIommuMapping(PMEMORY_DESCRIPTOR desc)
{
    return desc->needs_iommu;
}

static NV_STATUS
memdescUnmapIommuDupChecked(PMEMORY_DESCRIPTOR desc, NvU32 id)
{
    assert(desc != NULL && id == 7);
    checked_count++;
    return checked_result;
}

static void
memdescUnmapIommu(PMEMORY_DESCRIPTOR desc, NvU32 id)
{
    assert(desc != NULL && id == 7);
    legacy_count++;
}

#include "../../../src/nvidia/src/kernel/rmapi/nv_gpu_ops_iommu_impl.h"

int
main(void)
{
    MEMORY_DESCRIPTOR desc = {.needs_iommu = 1};
    Memory os_desc = {.pMemDesc = &desc, .is_os_descriptor = 1};
    Memory ordinary = {.pMemDesc = &desc, .is_os_descriptor = 0};

    checked_result = NV_ERR_OPERATING_SYSTEM;
    assert(nvGpuOpsUnmapDupIommu(&os_desc, 7, 1) == NV_ERR_OPERATING_SYSTEM);
    assert(checked_count == 1 && legacy_count == 0);

    checked_result = NV_OK;
    assert(nvGpuOpsUnmapDupIommu(&os_desc, 7, 1) == NV_OK);
    assert(checked_count == 2 && legacy_count == 0);

    assert(nvGpuOpsUnmapDupIommu(&ordinary, 7, 1) == NV_OK);
    assert(checked_count == 2 && legacy_count == 1);

    assert(nvGpuOpsUnmapDupIommu(&os_desc, 7, 0) == NV_OK);
    assert(checked_count == 2 && legacy_count == 2);

    desc.needs_iommu = 0;
    assert(nvGpuOpsUnmapDupIommu(&os_desc, 7, 1) == NV_OK);
    assert(checked_count == 2 && legacy_count == 2);
    return 0;
}
