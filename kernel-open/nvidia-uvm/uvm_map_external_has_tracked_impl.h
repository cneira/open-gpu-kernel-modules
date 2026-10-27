/* SPDX-License-Identifier: MIT */
/* Included by uvm_map_external.c and the tracked-range test. */
bool
uvm_ext_gpu_range_has_tracked_import(uvm_va_range_external_t *external_range,
                                      uvm_gpu_t *gpu)
{
    uvm_ext_gpu_range_tree_t *tree = uvm_ext_gpu_range_tree(external_range,
                                                              gpu);
    uvm_ext_gpu_map_t *map;

    uvm_assert_mutex_locked(&tree->lock);
    uvm_ext_gpu_map_for_each(map, external_range, gpu) {
        if (map->mem_handle != NULL && map->mem_handle->import_token != NULL)
            return true;
    }
    return false;
}
