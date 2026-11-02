# KPM Cache Attribute HAT Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the NVIDIA driver's unsafe KPM `hat_devload()` remap with an illumos HAT cache-bit update that preserves every unrelated PTE bit.

**Architecture:** The illumos HAT owns the PTE transition and verifies that an existing 4 KiB KPM mapping points to the requested PFN and has the expected cache policy. The driver calls it for WC and WB transitions, rolling back a partially converted allocation. Large KPM entries return `ENOTSUP`; splitting them requires a separate htable change and is outside this boot candidate.

**Tech Stack:** illumos x86 HAT, NVIDIA illumos kernel module, C, SmartOS native build.

**Spec:** `kernel-illumos/docs/kpm-wc-4k-experiment.md` and the recorded 2026-11-02 panic.

## Global Constraints

- Do not reboot Saruman without explicit user approval.
- Do not run GPU workloads with the failed diagnostic module loaded.
- New copyright: `Copyright 2026 Edgecast Cloud LLC.`
- Target the exact `joyent_20261028T004857Z` illumos source and NVIDIA 615.71.09 integration.

## Review Focus

- Preserve PT_GLOBAL, NX, PFN, permissions, ref/mod and software PTE bits.
- Reject large and absent KPM mappings without modifying them.
- Handle a changed PFN or cache state as an error rather than remapping it.
- Restore WB before driver-owned DMA memory is freed.
- Flush stale cache/TLB state at a policy transition.

---

### Task 1: HAT cache update

**Files:** `usr/src/uts/i86pc/vm/hat_i86.c`, `usr/src/uts/i86pc/vm/hat_i86.h` in illumos worktree.

**Interfaces:** Produce `int hat_kpm_set_cache(pfn_t, uint_t expected, uint_t desired)`; return zero or errno. Accept only `HAT_STORECACHING_OK` and `HAT_MERGING_OK`.

- [ ] Write a failing regression check using the panic PTE pair and require a cache-only operation.
- [ ] Run the check and confirm it fails for the existing implementation.
- [ ] Implement a 4 KiB-only CAS update of cache bits on the existing PTE.
- [ ] Run regression check and native syntax/build verification.
- [ ] Commit illumos change.

### Task 2: Driver integration

**Files:** `kernel-illumos/nvidia/nvidia_mem.c`, `kernel-illumos/docs/kpm-wc-4k-experiment.md` in driver worktree.

**Interfaces:** Consume `hat_kpm_set_cache` from Task 1; never call `hat_devload` to change KPM cache attributes.

- [ ] Write a failing check that detects any remaining KPM `hat_devload()` path.
- [ ] Run the check and confirm it fails.
- [ ] Replace WC/WB remapping, add partial-conversion rollback and transition flushes.
- [ ] Run check and native module build against patched illumos headers/kernel.
- [ ] Commit driver change and prepare staged artifacts; do not reboot.

### Task 3: Evidence and review

- [ ] Review the complete cross-repository diff for kernel safety.
- [ ] Update bead `sno-27t`, notes and validation evidence.
- [ ] Push the driver branch to the authorized Forgejo repository.
- [ ] Report exact build/test state and ask for a reboot only after a complete, reviewable candidate exists.
