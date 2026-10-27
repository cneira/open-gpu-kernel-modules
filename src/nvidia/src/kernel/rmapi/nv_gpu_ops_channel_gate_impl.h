/* SPDX-License-Identifier: MIT */
#ifndef NV_GPU_OPS_CHANNEL_GATE_IMPL_H
#define NV_GPU_OPS_CHANNEL_GATE_IMPL_H

/* STARTED blocks new work; STOPPED means STOP and GR eviction succeeded. */
#define NV_GPU_OPS_CHANNEL_REVOKE_STARTED  1U
#define NV_GPU_OPS_CHANNEL_REVOKE_STOPPED  2U

static inline NV_STATUS
nvGpuOpsChannelRevocationStatus(OBJVASPACE *pVAS, KernelChannel *pChannel)
{
    if (pChannel->pVAS != pVAS)
        return NV_OK;

    if ((portAtomicOrU32(&pChannel->uvmRevoked, 0) &
         (NV_GPU_OPS_CHANNEL_REVOKE_STARTED |
          NV_GPU_OPS_CHANNEL_REVOKE_STOPPED)) !=
        (NV_GPU_OPS_CHANNEL_REVOKE_STARTED |
         NV_GPU_OPS_CHANNEL_REVOKE_STOPPED))
        return NV_ERR_INVALID_STATE;

    return NV_OK;
}

#endif
