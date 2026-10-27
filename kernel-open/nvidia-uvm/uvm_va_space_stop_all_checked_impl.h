/* SPDX-License-Identifier: MIT */
/* Included by uvm_va_space.c and the focused channel-stop test. */
static NV_STATUS
uvm_va_space_stop_all_user_channels_status(uvm_va_space_t *va_space, bool immediate)
{
    uvm_gpu_va_space_t *gpu_va_space;
    uvm_user_channel_t *user_channel;
    NV_STATUS status = NV_OK;

    uvm_assert_rwsem_locked_read(&va_space->lock);

    if (atomic_read(&va_space->user_channels_stopped))
        return NV_OK;

    for_each_gpu_va_space(gpu_va_space, va_space) {
        list_for_each_entry(user_channel, &gpu_va_space->registered_channels, list_node) {
            NV_STATUS channel_status = uvm_user_channel_stop_mode_checked(user_channel, immediate);
            if (status == NV_OK)
                status = channel_status;
        }
    }

    /* Attempt every stop, but certify the set only if all RM calls passed. */
    if (status == NV_OK)
        atomic_set(&va_space->user_channels_stopped, 1);
    return status;
}

NV_STATUS
uvm_va_space_stop_all_user_channels_checked(uvm_va_space_t *va_space)
{
    return uvm_va_space_stop_all_user_channels_status(va_space, false);
}
