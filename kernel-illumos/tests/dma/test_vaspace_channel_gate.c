#include <assert.h>

typedef int NV_STATUS;
typedef int NvBool;
typedef unsigned int NvU32;
typedef struct OBJVASPACE { int unused; } OBJVASPACE;
typedef struct KernelChannel {
    OBJVASPACE *pVAS;
    volatile NvU32 uvmRevoked;
} KernelChannel;

#define NV_OK 0
#define NV_ERR_INVALID_STATE 1
#define NV_ERR_INVALID_CHANNEL 2
#define NV_WARN_MORE_PROCESSING_REQUIRED 3

static NvU32
portAtomicOrU32(volatile NvU32 *value, NvU32 mask)
{
    NvU32 old = *value;
    *value |= mask;
    return old;
}

#include "../../../src/nvidia/src/kernel/rmapi/nv_gpu_ops_channel_gate_impl.h"

int
main(void)
{
    OBJVASPACE target = {0}, other = {0};
    KernelChannel channel = {.pVAS = &other};

    assert(nvGpuOpsChannelRevocationStatus(&target, &channel) == NV_OK);
    channel.pVAS = &target;
    assert(nvGpuOpsChannelRevocationStatus(&target, &channel) == NV_ERR_INVALID_STATE);
    channel.uvmRevoked = NV_GPU_OPS_CHANNEL_REVOKE_STARTED;
    assert(nvGpuOpsChannelRevocationStatus(&target, &channel) == NV_ERR_INVALID_STATE);
    channel.uvmRevoked |= NV_GPU_OPS_CHANNEL_REVOKE_STOPPED;
    assert(nvGpuOpsChannelRevocationStatus(&target, &channel) == NV_OK);

    /* A missing retained instance needs the later global VA-space scan. */
    assert(nvGpuOpsMissingChannelRevokeStatus(NV_ERR_INVALID_CHANNEL, 1) ==
           NV_WARN_MORE_PROCESSING_REQUIRED);
    assert(nvGpuOpsMissingChannelRevokeStatus(NV_ERR_INVALID_CHANNEL, 0) ==
           NV_ERR_INVALID_CHANNEL);
    assert(nvGpuOpsMissingChannelRevokeStatus(NV_ERR_INVALID_STATE, 1) ==
           NV_ERR_INVALID_STATE);
    return 0;
}
