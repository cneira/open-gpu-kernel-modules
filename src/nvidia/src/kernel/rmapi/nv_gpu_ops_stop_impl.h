/* SPDX-License-Identifier: MIT */
/* Included by nv_gpu_ops.c and the focused channel-stop status test. */
static NV_STATUS
nvGpuOpsStopChannelControl(RM_API *pRmApi, KernelChannel *pKernelChannel,
                           gpuRetainedChannel *retainedChannel,
                           NvBool bImmediate)
{
    NVA06F_CTRL_STOP_CHANNEL_PARAMS stopChannelParams = {0};
    NV_STATUS status;

    stopChannelParams.bImmediate = bImmediate;
    status = pRmApi->Control(pRmApi,
                             RES_GET_CLIENT_HANDLE(pKernelChannel),
                             RES_GET_HANDLE(pKernelChannel),
                             NVA06F_CTRL_CMD_STOP_CHANNEL,
                             &stopChannelParams,
                             sizeof (stopChannelParams));
    if (status != NV_OK)
        return status;

    if (retainedChannel->channelEngineType == UVM_GPU_CHANNEL_ENGINE_TYPE_GR)
    {
        NV2080_CTRL_GPU_EVICT_CTX_PARAMS params;

        portMemSet(&params, 0, sizeof (params));
        params.engineType = NV2080_ENGINE_TYPE_GR(0);
        params.hClient = retainedChannel->session->handle;
        params.hChanClient = RES_GET_CLIENT_HANDLE(pKernelChannel);
        params.hObject = RES_GET_HANDLE(pKernelChannel);

        status = pRmApi->Control(pRmApi,
                                 retainedChannel->session->handle,
                                 retainedChannel->rmSubDevice->subDeviceHandle,
                                 NV2080_CTRL_CMD_GPU_EVICT_CTX,
                                 &params,
                                 sizeof (params));
        if (status != NV_OK)
            return status;
    }

    pKernelChannel->bIsContextBound = NV_FALSE;
    return NV_OK;
}
