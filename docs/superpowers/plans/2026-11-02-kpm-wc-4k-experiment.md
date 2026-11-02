# KPM WC 4K Experiment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a guarded driver diagnostic that mirrors eligible WC DMA pages in the KPM direct map while preserving NVDEC/NVENC cache requests.

**Architecture:** Keep the existing `nv_alloc_pages()` WC policy and UVM fixes. Preflight every KPM page for a matching 4 KiB mapping, remap eligible allocations through HAT, and restore write-back before release. Count skips so runtime evidence shows whether the target allocation was touched.

**Tech Stack:** illumos NVIDIA 615.71.09, x86 HAT, SmartOS `b01slow` build zone, CUDA reproducer.

**Spec:** `kernel-illumos/docs/kpm-wc-4k-experiment.md`

## Global Constraints

- Copyright for new project code: `Copyright 2026 Edgecast Cloud LLC.`
- No Saruman reboot without explicit operator approval.
- Preserve the integrated UVM pinned fixes and RM-requested WC cache type.
- Never remap a KPM large page through the driver.

## Review Focus

- A large or absent KPM mapping must cause no partial remap.
- A mismatched PFN must cause no partial remap.
- Teardown must restore WB before returning pages to the allocator.
- Allocation failure after remap must take the same restoration path.
- The media control must still pass on the candidate build.

### Task 1: Guarded driver remap

**Files:** `kernel-illumos/nvidia/nvidia_mem.c`, `kernel-illumos/nvidia/nv-illumos.h`

- [x] Confirm the existing fresh-process CUDA reproducer fails on the baseline (live record: iteration 3,950, 1,728 wrong bytes).
- [x] Add KPM preflight, all-CPU cache cleanup, same-PFN WC remap, and WB restoration on last-reference teardown.
- [x] Add visible counters for remapped and skipped pages, including allocation-failure cleanup.
- [x] Run native DMA tests and the exact-source SmartOS module build; check relocations and symbols.
- [x] Review the full diff against the spec and commit the diagnostic branch.

### Task 2: Controlled live candidate

**Files:** staging and rollback scripts in `smartos-nvidia-open-drivers-install`, live test report.

- [x] Verify candidate and installed module hashes, selected PI, pool, and rollback snapshot.
- [x] Stage only the changed `nvidia.kmod` beside the three matching integrated modules; do not reboot.
- [ ] After explicit reboot approval, verify live remap counters and three PTE types, then run the spec's H2D, pinned, Qwen, and media gates.
- [ ] Record raw logs, bead status, rollback decision, and exact loaded module set.
