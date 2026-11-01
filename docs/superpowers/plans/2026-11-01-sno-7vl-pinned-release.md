# CUDA pinned-host release implementation plan

> **For agentic workers:** Implement the tasks in order, testing each behavior before changing production code.

**Goal:** Releasing a pinned host mapping in a live LX CUDA process leaves unrelated GPU channels usable while retiring that mapping before its pin is unlocked.

**Architecture:** The import callback chooses targeted checked map teardown while its UVM VA space is live. VA-space teardown retains the existing global channel revoke and fault drain. Targeted teardown clears GPU PTEs with a system-memory TLB barrier before invalidating L2 and releasing the RM duplicate; failed barriers keep the pin.

**Tech Stack:** illumos NVIDIA RM/UVM C, focused C harnesses under `kernel-illumos/tests/dma`, Saruman LX CUDA lifecycle probe.

**Spec:** Bead `sno-7vl`; reproduction in `smartos-openwebui/tests/cuda_pinned_lifecycle.py`.

## Global constraints

- Do not replace the running module or reboot Saruman as part of patch construction.
- Keep the managed `GGML_CUDA_NO_PINNED=1` workaround until live acceptance tests pass.
- Preserve checked failure paths that retain the RM import and pin if any GPU barrier fails.

## Review focus

- A live callback must preserve user channels and retire every fragment of its imported mapping.
- VA-space teardown must still revoke channels and drain fault workers.
- A failed PTE, L2, page-tree, or RM release must retain the pin.
- Concurrent map publication, normal unmap, and callback ownership must remain serialized.
- On the real GPU, `cudaMallocHost` and `cudaHostRegister` lifecycle probes must survive three cycles before enabling pinned Qwen.

### Task 1: Focused red tests

**Files:** `kernel-illumos/tests/dma/test_uvm_import_revoke_checked.c`, `kernel-illumos/tests/dma/test_uvm_external_revoke.c`.

- [x] Add a live-VA scenario asserting targeted map destruction succeeds without global channel revoke or fault-worker detachment.
- [x] Add a teardown scenario asserting the existing full revoke and fault drain still run.
- [x] Assert PTE clear completes before L2 invalidation and a failed barrier preserves the map.
- [x] Run the two focused tests and observe the expected failures.

### Task 2: Minimal driver change

**Files:** `kernel-open/nvidia-uvm/uvm_map_external_import_checked_impl.h`, `kernel-open/nvidia-uvm/uvm_map_external_revoke_impl.h`, `kernel-open/nvidia-uvm/uvm_va_space.c`.

- [x] Branch between live targeted teardown and full VA-space teardown under the VA lock.
- [x] Mark file-release teardown before revoking imports.
- [x] Clear PTEs and wait for the TLB memory barrier before L2 invalidation and RM release.
- [x] Run focused tests, then `make -C kernel-illumos/tests/dma check`.

### Task 3: Integration and handoff

- [x] Build the illumos driver modules in the existing build zone and check module linkage.
- [x] Record remaining live validation in `sno-7vl`, commit and push the patch branch.

## Build evidence

Commit `2b54ee3` built in `b01slow` against the headers for Saruman's
`joyent_20261028T004857Z` kernel. The source archive SHA-256 was
`2d01bdb9cebff1534a629d4503dc6dbc636b5d3f18a5bac88ae52ffc1c87a2a7`.
All four modules linked. The crypto test passed 7,394 checks with zero
failures. `elfdump -r` found zero nondebug `R_AMD64_32` relocations in each
module. The candidate `nvidia_uvm.kmod` SHA-256 is
`6c8ea80ed3c3b817b47768a43d0e7e70592adefa3ea2ba2545aa504172655102`.
Build logs and objects are confined to `/var/tmp/sno-7vl-2b54ee3` in
`b01slow`; no production module was replaced.

The review bundle is `/tmp/smartos-nvidia-615.71.09-sno-7vl-2b54ee3.tar.gz`
on the workstation, SHA-256
`74c3861519b7dc9c866f5296c66062f670733acb272c3df0437ce03c5d735587`.
Its manifest pins driver `2b54ee3`, illumos
`860a3aca2c761b58119d369f18efb837b9331747`, kernel
`joyent_20261028T004857Z`, and PCI device `2803`. The extracted UVM
module hash matches the build output. Live CUDA and zone-stop validation
remains open in `sno-7vl`.
