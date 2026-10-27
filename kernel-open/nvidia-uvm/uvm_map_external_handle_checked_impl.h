/* SPDX-License-Identifier: MIT */
/* Included by uvm_map_external.c and the focused failed-RM-free test. */
static NV_STATUS
uvm_release_rm_handle_checked(uvm_ext_gpu_mem_handle *mem_handle)
{
    NV_STATUS status;

    if (mem_handle == NULL || mem_handle->gpu == NULL ||
        mem_handle->rm_handle == 0 || mem_handle->checked_release_failed ||
        nv_kref_read(&mem_handle->ref_count) != 1)
        return NV_ERR_INVALID_STATE;

    status = uvm_rm_locked_call(nvUvmInterfaceFreeDupedHandle(
        uvm_gpu_device_handle(mem_handle->gpu), mem_handle->rm_handle));
    if (status != NV_OK) {
        /* RM may have partly freed the handle; retry is not provably safe. */
        mem_handle->checked_release_failed = true;
        return status;
    }

    mem_handle->rm_handle = 0;
    nv_kref_put(&mem_handle->ref_count, uvm_release_rm_handle);
    return NV_OK;
}
