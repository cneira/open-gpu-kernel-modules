/* SPDX-License-Identifier: MIT */
/* Included by uvm_map_external.c and the focused tracked-destroy test. */
NV_STATUS
uvm_ext_gpu_map_destroy(uvm_va_range_external_t *external_range,
                        uvm_ext_gpu_map_t *ext_gpu_map,
                        struct list_head *deferred_free_list)
{
    if (ext_gpu_map != NULL && ext_gpu_map->mem_handle != NULL &&
        ext_gpu_map->mem_handle->import_token != NULL)
        return uvm_ext_gpu_map_destroy_tracked(external_range, ext_gpu_map);

    uvm_ext_gpu_map_destroy_legacy(external_range, ext_gpu_map,
                                    deferred_free_list);
    return NV_OK;
}
