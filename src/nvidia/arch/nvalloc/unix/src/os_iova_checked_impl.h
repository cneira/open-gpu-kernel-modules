/* SPDX-License-Identifier: MIT */
/* Included by os.c and by the focused unbind-failure test. */
NV_STATUS
osIovaUnmapAllocationChecked(PIOVAMAPPING pIovaMapping,
                             nv_dma_device_t *dmaDevice,
                             NvU64 osPageCount)
{
    void *pPriv;
    NV_STATUS status;

    if (pIovaMapping == NULL || pIovaMapping->pOsData == NULL)
        return NV_ERR_NOT_SUPPORTED;

    pPriv = (void *)pIovaMapping->pOsData;
    status = nv_dma_unmap_alloc(dmaDevice, osPageCount,
                                &pIovaMapping->iovaArray[0], &pPriv);
    if (status != NV_OK)
        return status;

    pIovaMapping->pOsData = pPriv;
    return NV_OK;
}
