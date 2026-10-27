/* Stop and GR eviction failures must leave the channel state unconfirmed. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef int NV_STATUS;
typedef int NvBool;
typedef unsigned int NvU32;
typedef struct { NvBool bImmediate; } NVA06F_CTRL_STOP_CHANNEL_PARAMS;
typedef struct {
    NvU32 engineType;
    NvU32 hClient;
    NvU32 hChanClient;
    NvU32 hObject;
} NV2080_CTRL_GPU_EVICT_CTX_PARAMS;
typedef struct { NvU32 handle; } TEST_SESSION;
typedef struct { NvU32 subDeviceHandle; } TEST_SUBDEVICE;
typedef struct {
    TEST_SESSION *session;
    TEST_SUBDEVICE *rmSubDevice;
    NvU32 channelEngineType;
} gpuRetainedChannel;
typedef struct {
    NvBool bIsContextBound;
    NvU32 client;
    NvU32 handle;
} KernelChannel;
typedef struct RM_API RM_API;
struct RM_API {
    NV_STATUS (*Control)(RM_API *, NvU32, NvU32, NvU32, void *, size_t);
};

#define NV_OK 0
#define NV_ERR_OPERATING_SYSTEM 1
#define NV_FALSE 0
#define NV_TRUE 1
#define NVA06F_CTRL_CMD_STOP_CHANNEL 10
#define NV2080_CTRL_CMD_GPU_EVICT_CTX 20
#define UVM_GPU_CHANNEL_ENGINE_TYPE_GR 30
#define NV2080_ENGINE_TYPE_GR(x) (40 + (x))
#define RES_GET_CLIENT_HANDLE(channel) ((channel)->client)
#define RES_GET_HANDLE(channel) ((channel)->handle)
#define portMemSet memset

static NV_STATUS stop_status;
static NV_STATUS evict_status;
static int stop_count;
static int evict_count;
static NvBool seen_immediate;

static NV_STATUS
control(RM_API *api, NvU32 client, NvU32 handle, NvU32 command,
        void *params, size_t size)
{
    (void)api;
    (void)client;
    (void)handle;
    if (command == NVA06F_CTRL_CMD_STOP_CHANNEL) {
        assert(size == sizeof (NVA06F_CTRL_STOP_CHANNEL_PARAMS));
        seen_immediate = ((NVA06F_CTRL_STOP_CHANNEL_PARAMS *)params)->bImmediate;
        stop_count++;
        return stop_status;
    }
    assert(command == NV2080_CTRL_CMD_GPU_EVICT_CTX);
    assert(size == sizeof (NV2080_CTRL_GPU_EVICT_CTX_PARAMS));
    evict_count++;
    return evict_status;
}

#include "../../../src/nvidia/src/kernel/rmapi/nv_gpu_ops_stop_impl.h"

int
main(void)
{
    RM_API api = {.Control = control};
    TEST_SESSION session = {.handle = 1};
    TEST_SUBDEVICE subdevice = {.subDeviceHandle = 2};
    gpuRetainedChannel retained = {
        .session = &session,
        .rmSubDevice = &subdevice,
        .channelEngineType = UVM_GPU_CHANNEL_ENGINE_TYPE_GR,
    };
    KernelChannel channel = {
        .bIsContextBound = NV_TRUE,
        .client = 3,
        .handle = 4,
    };

    stop_status = NV_ERR_OPERATING_SYSTEM;
    assert(nvGpuOpsStopChannelControl(&api, &channel, &retained,
                                      NV_FALSE) == NV_ERR_OPERATING_SYSTEM);
    assert(channel.bIsContextBound == NV_TRUE);
    assert(stop_count == 1 && evict_count == 0 && seen_immediate == NV_FALSE);

    stop_status = NV_OK;
    evict_status = NV_ERR_OPERATING_SYSTEM;
    assert(nvGpuOpsStopChannelControl(&api, &channel, &retained,
                                      NV_FALSE) == NV_ERR_OPERATING_SYSTEM);
    assert(channel.bIsContextBound == NV_TRUE);
    assert(stop_count == 2 && evict_count == 1);

    evict_status = NV_OK;
    assert(nvGpuOpsStopChannelControl(&api, &channel, &retained,
                                      NV_FALSE) == NV_OK);
    assert(channel.bIsContextBound == NV_FALSE);
    assert(stop_count == 3 && evict_count == 2);

    channel.bIsContextBound = NV_TRUE;
    retained.channelEngineType = 99;
    assert(nvGpuOpsStopChannelControl(&api, &channel, &retained,
                                      NV_FALSE) == NV_OK);
    assert(channel.bIsContextBound == NV_FALSE);
    assert(stop_count == 4 && evict_count == 2);
    return 0;
}
