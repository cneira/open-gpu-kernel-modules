/* A failed channel stop must not certify a VA space as stopped. */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef int NV_STATUS;
#define NV_OK 0
#define NV_ERR_OPERATING_SYSTEM 1

typedef struct {
    int stop_result;
    int stop_calls;
    bool last_immediate;
} uvm_user_channel_t;

typedef struct {
    struct {
        uvm_user_channel_t *channels;
        int channel_count;
    } registered_channels;
} uvm_gpu_va_space_t;

typedef struct {
    uvm_gpu_va_space_t *gpus;
    int gpu_count;
    int user_channels_stopped;
    int lock;
} uvm_va_space_t;

#define for_each_gpu_va_space(gpu, va) \
    for (int gpu_index = 0; gpu_index < (va)->gpu_count && \
        (((gpu) = &(va)->gpus[gpu_index]) != NULL); gpu_index++)
#define list_for_each_entry(channel, head, member) \
    for (int channel_index = 0; channel_index < (head)->channel_count && \
        (((channel) = &(head)->channels[channel_index]) != NULL); channel_index++)

static int atomic_read(int *value) { return *value; }
static void atomic_set(int *value, int new_value) { *value = new_value; }
static void uvm_assert_rwsem_locked_read(int *lock) { assert(lock != NULL); }
static NV_STATUS
uvm_user_channel_stop_mode_checked(uvm_user_channel_t *channel, bool immediate)
{
    channel->stop_calls++;
    channel->last_immediate = immediate;
    return channel->stop_result;
}

#include "../../../kernel-open/nvidia-uvm/uvm_va_space_stop_all_checked_impl.h"

int
main(void)
{
    uvm_user_channel_t channels[] = {
        {0, 0, false}, {NV_ERR_OPERATING_SYSTEM, 0, false}, {0, 0, false}
    };
    uvm_gpu_va_space_t gpu = {{channels, 3}};
    uvm_va_space_t va = {&gpu, 1, 0, 0};

    assert(uvm_va_space_stop_all_user_channels_checked(&va) ==
           NV_ERR_OPERATING_SYSTEM);
    assert(va.user_channels_stopped == 0);
    assert(channels[0].stop_calls == 1);
    assert(channels[1].stop_calls == 1);
    assert(channels[2].stop_calls == 1);
    assert(!channels[0].last_immediate && !channels[1].last_immediate);

    channels[1].stop_result = NV_OK;
    assert(uvm_va_space_stop_all_user_channels_checked(&va) == NV_OK);
    assert(va.user_channels_stopped == 1);
    assert(channels[0].stop_calls == 2);
    assert(channels[1].stop_calls == 2);
    assert(channels[2].stop_calls == 2);

    assert(uvm_va_space_stop_all_user_channels_checked(&va) == NV_OK);
    assert(channels[0].stop_calls == 2);

    va.user_channels_stopped = 0;
    channels[1].stop_result = NV_ERR_OPERATING_SYSTEM;
    assert(uvm_va_space_stop_all_user_channels_status(&va, true) ==
           NV_ERR_OPERATING_SYSTEM);
    assert(va.user_channels_stopped == 0);
    assert(channels[0].last_immediate && channels[1].last_immediate);
    return 0;
}
