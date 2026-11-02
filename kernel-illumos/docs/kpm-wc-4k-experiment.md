<!-- Copyright 2026 Edgecast Cloud LLC. -->

# Guarded KPM cache-attribute experiment for CUDA upload pages

## Question

Does the write-back KPM alias of a driver-owned write-combined (WC) page cause
Saruman's intermittent stale CUDA host-to-device data? The current driver
passes NVDEC/NVENC and pinned-host allocation lifecycle tests, but a fresh
pageable CUDA copy process observed 1,728 old bytes at transfer 3,950 and a
pinned Qwen model produced incorrect output. The exact live evidence is in
`smartos-nvidia-open-drivers-install/benchmarks/sno-7vl-dtrace-20261101/`.

## Design

Keep RM's requested WC policy and the existing DDI and user mappings. For
WC system RAM allocated through `nv_alloc_pages()`, inspect every physical
page's KPM mapping before changing any of them. Require an existing 4 KiB
mapping of the same PFN. If any page fails that check, leave the entire
allocation unchanged and count the skip. This diagnostic build must never
attempt to overwrite a large KPM mapping: illumos `htable_create()` panics
if it links a 4 KiB table over a present large-page entry.

For eligible allocations, flush CPU caches on all CPUs before changing the
KPM entries, then use `hat_devload()` to remap each KPM 4 KiB entry to WC.
`HAT_PLAT_NOCACHE` prevents `hat_devload()` from forcing system RAM back to
write-back; `HAT_MERGING_OK` selects WC via PAT. The same-PFN remap changes
only permitted PTE bits, and the HAT path shoots down the kernel TLB. Keep
the allocation marked as remapped so its last-reference destruction flushes
again and restores KPM write-back before `ddi_dma_mem_free()` releases pages.
Do not touch BAR, peer, user-pinned, or alias allocations.

This is a guarded diagnostic, not a production cache policy. It uses a full
CPU cache flush per transition and does not handle large KPM mappings. A
measured skip on the CUDA staging allocation means this test is inconclusive
and a small illumos HAT operation with safe large-page demotion is needed.
The preflight assumes the KPM topology of allocated DMA pages stays stable
through the remap. Restoration assumes the pre-existing KPM entry was
write-back, as observed on Saruman; public HAT queries do not report its
cache mode. The exact PI's `hat_i86.h` defines the private no-cache override
as `0x100000`, which the external module uses without including that
machine-dependent header. Verify these assumptions with mdb on the live
candidate before accepting any CUDA result.

## Acceptance and rollback

The existing `cuda_copy_integrity` reproducer failed on the pre-policy
candidate before this patch. On a separately approved boot, inspect the
live KPM, DDI, and user PTEs for the same staging PFN; verify the diagnostic
counter confirms an actual remap. Then run repeated fresh-process pageable
H2D copies, pinned lifecycle and pinned Qwen correctness, the exact 30-frame
NVDEC/NVENC control at idle and with Qwen active, and check for Xid or
teardown errors. A passing run would support the alias hypothesis but still
needs independent review and broader soak before production acceptance.

Only `nvidia.kmod` changes; the other three integrated modules stay paired.
Before another reboot, back up the current module, take a `slow/boot` ZFS
snapshot, and stage the candidate with exact hashes. Do not reboot without
the operator's explicit approval. Restore the backed-up module if this
experiment fails; that restoration also requires an approved reboot to load.

## Result: do not deploy this driver-only experiment

The approved boot loaded this diagnostic, but the first idle 30-frame
NVDEC/NVENC control panicked Saruman at 2026-11-02 16:53:52 UTC. The saved
panic is `remap bits changed: old_pte=0xa071523,
pte=0x800000000a0714f3`. The only changed bit outside native
`PT_REMAP_BITS` is `PT_GLOBAL` (bit 8): it was present in the KPM entry and
absent in the entry rebuilt by `hat_devload()`. Native `PT_IGNORE` is zero,
and the loaded kernel's `mmu.pt_global` is zero under KPTI. The exact HAT
implementation calls `x86pte_set()` before checking the remap bits, so this
path writes the unsuitable PTE and then panics. The pageable H2D and pinned
Qwen tests were not reached. This experiment establishes a driver-only
remapping failure; it does not establish whether the original WB KPM alias
causes stale CUDA uploads.

The prior module and Qwen configuration are restored on disk. The failed
module remains loaded until a separately approved recovery reboot. The
compressed dump is `/slow/crash/vmdump.5`; `savecore -f` reported an
incomplete dump and left an unreadable partial core. See the
`sno-7vl-dtrace-20261101` installer report for raw logs.

The next patch belongs in illumos HAT. It needs an explicit direct-map
cache-attribute operation that preserves all non-cache PTE bits, including
`PT_GLOBAL`, changes only the PAT/cache bits, performs cache and TLB
maintenance, and safely demotes large KPM entries when changing only part
of a large page. A generic `hat_devload()` remap and a blanket allowance to
toggle `PT_GLOBAL` are not substitutes for that operation.
