/*
 * SPDX-License-Identifier: MIT
 *
 * Kept separate so the DMA unmap failure path can be exercised with DDI
 * stubs. The including file provides nv_dma_map_t and the DDI/NVIDIA types.
 */
NV_STATUS NV_API_CALL nv_dma_unmap_alloc(
    nv_dma_device_t *dma_dev,
    NvU64            page_count,
    NvU64           *va_array,
    void           **priv
)
{
    nv_dma_map_t *map;

    (void)dma_dev;
    (void)page_count;
    (void)va_array;

    if (priv == NULL || *priv == NULL)
        return NV_ERR_NOT_SUPPORTED;

    map = *priv;

    if (map->handle != NULL)
    {
        if (ddi_dma_unbind_handle(map->handle) != DDI_SUCCESS)
            return NV_ERR_OPERATING_SYSTEM;
        ddi_dma_free_handle(&map->handle);
    }

    if (map->own_kva)
        nv_kunmap_phys(map->kva, map->page_count);

    kmem_free(map, sizeof (*map));
    *priv = NULL;
    return NV_OK;
}
