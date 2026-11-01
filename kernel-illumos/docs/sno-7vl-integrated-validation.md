# Saruman integrated NVIDIA validation candidate

This branch combines the driver changes for one supervised test on the
`joyent_20261028T004857Z` SmartOS platform image (illumos gate
`860a3aca2c761b58119d369f18efb837b9331747`), NVIDIA 615.71.09, and
GPU PCI ID `2803`. Build and package the exact branch commit; do not combine
modules from different builds.

## Current status

Commit `12a5c28` builds, but is not ready for live deployment or merge.
The deferred external-range patch still permits ordinary and sparse external
mapping into a range whose `UVM_FREE` is pending. Replacement mapping may
remove the old map and fail before creating a new one, leaving the pending
range in the VA tree; successful replacement retains the pending state.
Block these mappings or implement safe cancellation, cover replacement
failure with a regression test, then repeat the exact-kernel build and live
acceptance sequence below. Keep bead `sno-7vl` open.

## Included driver behavior

| Area | Source history | Validation target |
| --- | --- | --- |
| Checked UVM/RM shutdown and zone halt | `6012cca`, including the later `sno-jow` lock and failure handling behavior | Interrupted NVENC zone stop and other-zone GPU access |
| CUDA cache alias policy | `59aff83` | Pageable host-to-device copies and Qwen response |
| Pinned-host import release | `2b54ee3` | `cudaMallocHost` and `cudaHostRegister` lifecycle |
| Boot framebuffer console and PCI handle promotion | Merge of local `illumos` branch at `fbaf0dc` with upstream console fixes | Boot console, VIS ioctl gate, PCI handle cache |

The `cuda-identity-dma` branch (`7044902`) is deliberately excluded: its
live experiment failed. The current platform image already contains the LX
kernel fixes; no kernel replacement is part of this candidate. The managed
OpenWebUI/Qwen service keeps `GGML_CUDA_NO_PINNED=1` until pinned-host tests
pass on the candidate modules.

## Live acceptance sequence

1. Confirm the platform image and four installed module hashes match the
   bundle manifest before booting the candidate.
2. Check the framebuffer console and kernel-only VIS path, then inspect the
   PCI handle cache for unique BDFs and a populated GPU dip/config handle.
3. Run `tests/cuda_pinned_lifecycle.py` from smartos-openwebui in malloc and
   register modes for three cycles each. Confirm that unpin completes after
   unregister/free and every subsequent synchronization succeeds without
   CUDA 719.
4. Run pageable CUDA copy probes and Qwen pinned startup, response, and clean
   shutdown. Only then remove the managed no-pinned workaround.
5. Interrupt NVENC in an LX zone, stop that zone, and confirm another LX zone
   can still use the GPU.

Keep the previous module bundle and service configuration available for
rollback. Record module hashes, probe output, and host logs against bead
`sno-7vl` before declaring the driver production ready.
