/*
 * SPDX-FileCopyrightText: Copyright 2026 Edgecast Cloud LLC.
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef int NvBool;
typedef void *ddi_dma_handle_t;
typedef void *ddi_acc_handle_t;

#define NV_TRUE 1
#define NV_FALSE 0
#define DDI_SUCCESS 0

static int unbind_result;
static int unbind_calls;
static int mem_free_calls;
static int handle_free_calls;

static int
ddi_dma_unbind_handle(ddi_dma_handle_t handle)
{
    assert(handle == (void *)(uintptr_t)0x1234);
    unbind_calls++;
    return unbind_result;
}

static void
ddi_dma_mem_free(ddi_acc_handle_t *acc)
{
    assert(*acc == (void *)(uintptr_t)0x5678);
    mem_free_calls++;
    *acc = NULL;
}

static void
ddi_dma_free_handle(ddi_dma_handle_t *handle)
{
    assert(*handle == (void *)(uintptr_t)0x1234);
    handle_free_calls++;
    *handle = NULL;
}

#include "../../nvidia/nvidia_dma_probe_impl.h"

static void
reset_counts(void)
{
    unbind_calls = 0;
    mem_free_calls = 0;
    handle_free_calls = 0;
    unbind_result = DDI_SUCCESS;
}

int
main(void)
{
    ddi_dma_handle_t h = (void *)(uintptr_t)0x1234;
    ddi_acc_handle_t acc = (void *)(uintptr_t)0x5678;

    reset_counts();
    unbind_result = -1;
    assert(nv_dma_probe_release(&h, &acc, NV_TRUE) == NV_FALSE);
    assert(h != NULL && acc != NULL);
    assert(unbind_calls == 1 && mem_free_calls == 0 &&
        handle_free_calls == 0);

    reset_counts();
    assert(nv_dma_probe_release(&h, &acc, NV_TRUE) == NV_TRUE);
    assert(h == NULL && acc == NULL);
    assert(unbind_calls == 1 && mem_free_calls == 1 &&
        handle_free_calls == 1);

    h = (void *)(uintptr_t)0x1234;
    acc = (void *)(uintptr_t)0x5678;
    reset_counts();
    assert(nv_dma_probe_release(&h, &acc, NV_FALSE) == NV_TRUE);
    assert(unbind_calls == 0 && mem_free_calls == 1 &&
        handle_free_calls == 1);
    return 0;
}
