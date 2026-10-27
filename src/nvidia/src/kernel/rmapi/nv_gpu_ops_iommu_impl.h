/* SPDX-License-Identifier: MIT */
/* Included by nv_gpu_ops.c and by the focused OS-descriptor release test. */
static NV_STATUS
nvGpuOpsUnmapDupIommu(Memory *pMemory, NvU32 iovaspaceId, NvBool bChecked)
{
    PMEMORY_DESCRIPTOR pMemDesc = pMemory->pMemDesc;

    if (!memdescRequiresIommuMapping(pMemDesc))
        return NV_OK;

#if defined(NV_SUNOS)
    if (bChecked && dynamicCast(pMemory, OsDescMemory) != NULL)
        return memdescUnmapIommuChecked(pMemDesc, iovaspaceId);
#else
    (void)bChecked;
#endif

    memdescUnmapIommu(pMemDesc, iovaspaceId);
    return NV_OK;
}
