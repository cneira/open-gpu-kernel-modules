/*
 * SPDX-FileCopyrightText: Copyright 2026 Edgecast Cloud LLC.
 * SPDX-License-Identifier: MIT
 */

/* Exercise the driver's identity-bind and release code with DDI substitutes. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t NvU64;
typedef int NvBool;
typedef char *caddr_t;
typedef void *ddi_acc_handle_t;
typedef void *ddi_dma_handle_t;
typedef unsigned int uint_t;

#define NV_TRUE 1
#define NV_FALSE 0
#define MMU_PAGESIZE 4096
#define DDI_SUCCESS 0
#define DDI_DMA_MAPPED 1
#define DDI_DMA_PARTIAL_MAP 9
#define DDI_DMA_RDWR 2
#define DDI_DMA_CONSISTENT 4
#define DMP_NOSYNC 8

typedef struct ddi_dma_cookie {
    NvU64 dmac_laddress;
    size_t dmac_size;
} ddi_dma_cookie_t;

typedef struct ddi_dma_impl {
    uint_t dmai_rflags;
} ddi_dma_impl_t;

typedef struct nv_dma_chunk {
    ddi_dma_handle_t dma_handle;
    ddi_acc_handle_t acc_handle;
    caddr_t kva;
    size_t len;
    NvBool bound;
} nv_dma_chunk_t;

static ddi_dma_impl_t impl;
static ddi_dma_impl_t impl2;
static ddi_dma_cookie_t cookies[3];
static uint_t ncookies;
static uint_t next_cookie;
static int bind_result;
static int unbind_result;
static int unbind_result2;
static int unbind_calls;
static int mem_free_calls;
static int handle_free_calls;
static int retained_calls;
static int retained_second;
static char events[16];
static int event_count;

static NvU64
nv_kva_to_phys(caddr_t kva)
{
    return 0x100000 + (uintptr_t)kva - 0x400000;
}

static int
ddi_dma_addr_bind_handle(ddi_dma_handle_t handle, void *as, caddr_t kva,
    size_t len, uint_t flags, int (*waitfp)(caddr_t), void *arg,
    ddi_dma_cookie_t *cookie, uint_t *count)
{
    assert(handle == &impl);
    assert(as == NULL && arg == NULL && waitfp == NULL);
    assert(kva == (caddr_t)(uintptr_t)0x400000);
    assert(len == 8192);
    assert(flags == (DDI_DMA_RDWR | DDI_DMA_CONSISTENT));
    *cookie = cookies[0];
    *count = ncookies;
    next_cookie = 1;
    return bind_result;
}

static void
ddi_dma_nextcookie(ddi_dma_handle_t handle, ddi_dma_cookie_t *cookie)
{
    assert(handle == &impl);
    assert(next_cookie < ncookies);
    *cookie = cookies[next_cookie++];
}

static int
ddi_dma_unbind_handle(ddi_dma_handle_t handle)
{
    assert(handle == &impl || handle == &impl2);
    unbind_calls++;
    events[event_count++] = 'U';
    return handle == &impl ? unbind_result : unbind_result2;
}

static void
ddi_dma_mem_free(ddi_acc_handle_t *acc)
{
    assert(*acc == (void *)(uintptr_t)0x1234 ||
        *acc == (void *)(uintptr_t)0x5678);
    mem_free_calls++;
    events[event_count++] = 'M';
    *acc = NULL;
}

static void
ddi_dma_free_handle(ddi_dma_handle_t *handle)
{
    assert(*handle == &impl || *handle == &impl2);
    handle_free_calls++;
    events[event_count++] = 'H';
    *handle = NULL;
}

#include "../../nvidia/nvidia_mem_dma_impl.h"

static nv_dma_chunk_t
new_chunk(void)
{
    nv_dma_chunk_t chunk = {
        .dma_handle = &impl,
        .acc_handle = (void *)(uintptr_t)0x1234,
        .kva = (caddr_t)(uintptr_t)0x400000,
        .len = 8192
    };

    memset(cookies, 0, sizeof (cookies));
    cookies[0].dmac_laddress = 0x100000;
    cookies[0].dmac_size = 8192;
    ncookies = 1;
    impl.dmai_rflags = DMP_NOSYNC;
    bind_result = DDI_DMA_MAPPED;
    unbind_result = DDI_SUCCESS;
    unbind_result2 = DDI_SUCCESS;
    unbind_calls = 0;
    mem_free_calls = 0;
    handle_free_calls = 0;
    retained_calls = 0;
    retained_second = 0;
    event_count = 0;
    return chunk;
}

static void
record_retained(nv_dma_chunk_t *c)
{
    retained_calls++;
    retained_second = c->dma_handle == &impl2;
    events[event_count++] = 'R';
}

int
main(void)
{
    nv_dma_chunk_t chunk = new_chunk();
    nv_dma_chunk_t chunks[3];
    uint_t retained_count;
    NvU64 retained_bytes;

    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_TRUE);
    assert(chunk.bound == NV_TRUE);
    assert(nv_dma_release_chunk(&chunk) == NV_TRUE);
    assert(unbind_calls == 1 && mem_free_calls == 1 && handle_free_calls == 1);

    chunk = new_chunk();
    cookies[0].dmac_size = 4096;
    cookies[1].dmac_laddress = 0x101000;
    cookies[1].dmac_size = 4096;
    ncookies = 2;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_TRUE);
    assert(next_cookie == 2);

    chunk = new_chunk();
    cookies[0].dmac_laddress += 4096;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);
    assert(chunk.bound == NV_FALSE && unbind_calls == 1);

    chunk = new_chunk();
    cookies[0].dmac_size = 4096;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);
    assert(chunk.bound == NV_FALSE && unbind_calls == 1);

    chunk = new_chunk();
    cookies[0].dmac_size = 0;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);

    chunk = new_chunk();
    cookies[0].dmac_laddress = UINT64_MAX - 4095;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);

    chunk = new_chunk();
    impl.dmai_rflags = 0;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);

    chunk = new_chunk();
    unbind_result = -1;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_TRUE);
    assert(nv_dma_release_chunk(&chunk) == NV_FALSE);
    assert(chunk.bound == NV_TRUE);
    assert(chunk.acc_handle != NULL && chunk.dma_handle != NULL);
    assert(mem_free_calls == 0 && handle_free_calls == 0);

    chunk = new_chunk();
    cookies[0].dmac_laddress += 4096;
    unbind_result = -1;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);
    assert(chunk.bound == NV_TRUE);
    assert(nv_dma_release_chunk(&chunk) == NV_FALSE);
    assert(mem_free_calls == 0 && handle_free_calls == 0);

    chunk = new_chunk();
    bind_result = DDI_DMA_PARTIAL_MAP;
    unbind_result = -1;
    assert(nv_dma_identity_bind_chunk(&chunk, NULL) == NV_FALSE);
    assert(chunk.bound == NV_TRUE);
    assert(nv_dma_release_chunk(&chunk) == NV_FALSE);
    assert(mem_free_calls == 0 && handle_free_calls == 0);

    chunks[0] = new_chunk();
    chunks[0].bound = NV_TRUE;
    chunks[1] = chunks[0];
    chunks[1].dma_handle = &impl2;
    chunks[1].acc_handle = (void *)(uintptr_t)0x5678;
    chunks[2] = (nv_dma_chunk_t){ 0 };
    unbind_result2 = -1;
    nv_dma_release_chunks(chunks, 3, record_retained,
        &retained_count, &retained_bytes);
    assert(chunks[0].dma_handle == NULL && chunks[0].acc_handle == NULL);
    assert(chunks[1].dma_handle == &impl2 &&
        chunks[1].acc_handle == (void *)(uintptr_t)0x5678);
    assert(chunks[2].dma_handle == NULL && chunks[2].acc_handle == NULL);
    assert(unbind_calls == 2 && mem_free_calls == 1 &&
        handle_free_calls == 1);
    assert(retained_calls == 1 && retained_second);
    assert(retained_count == 1 && retained_bytes == 8192);
    assert(event_count == 5 && memcmp(events, "UMHUR", 5) == 0);

    return 0;
}
