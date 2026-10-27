/* SPDX-License-Identifier: MIT */
/* Included by uvm_va_space.c and the focused RM failure test. */
NV_STATUS
uvm_gpu_va_space_unset_page_dir_checked(uvm_gpu_va_space_t *gpu_va_space)
{
    NV_STATUS status;

    if (uvm_gpu_va_space_state(gpu_va_space) != UVM_GPU_VA_SPACE_STATE_INIT)
        uvm_assert_rwsem_locked_read(&gpu_va_space->va_space->lock);

    if (!gpu_va_space->did_set_page_directory)
        return NV_OK;

    status = uvm_rm_locked_call(nvUvmInterfaceUnsetPageDirectory(
        gpu_va_space->duped_gpu_va_space));
    if (status == NV_OK)
        gpu_va_space->did_set_page_directory = false;

    return status;
}
