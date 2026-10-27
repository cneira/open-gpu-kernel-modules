/* SPDX-License-Identifier: MIT */
/* Included by uvm_va_space.c and the focused channel-stop test. */
NV_STATUS
uvm_va_space_stop_all_user_channels_checked(uvm_va_space_t *va_space)
{
    uvm_gpu_va_space_t *gpu_va_space;
    uvm_user_channel_t *user_channel;
    NV_STATUS status;

    uvm_assert_rwsem_locked_read(&va_space->lock);

    if (atomic_read(&va_space->user_channels_stopped))
        return NV_OK;

    for_each_gpu_va_space(gpu_va_space, va_space) {
        list_for_each_entry(user_channel, &gpu_va_space->registered_channels, list_node) {
            status = uvm_user_channel_stop_checked(user_channel);
            if (status != NV_OK)
                return status;
        }
    }

    /* A failed stop never certifies the whole VA space as quiescent. */
    atomic_set(&va_space->user_channels_stopped, 1);
    return NV_OK;
}
