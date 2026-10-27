/* SPDX-License-Identifier: MIT */
#ifndef NV_GPU_OPS_CHANNEL_GATE_IMPL_H
#define NV_GPU_OPS_CHANNEL_GATE_IMPL_H

/* STARTED blocks new work; STOPPED means STOP and GR eviction succeeded. */
#define NV_GPU_OPS_CHANNEL_REVOKE_STARTED  1U
#define NV_GPU_OPS_CHANNEL_REVOKE_STOPPED  2U

/*
 * A retained instance may disappear when the user frees its RM channel
 * during process exit.  Only the lookup failure can be handed to the later
 * all-client VA-space scan; a failure from the stop control stays fatal.
 */
static inline NV_STATUS
nvGpuOpsMissingChannelRevokeStatus(NV_STATUS lookupStatus, NvBool bRevoke)
{
    if (bRevoke && lookupStatus == NV_ERR_INVALID_CHANNEL)
        return NV_WARN_MORE_PROCESSING_REQUIRED;
    return lookupStatus;
}

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
