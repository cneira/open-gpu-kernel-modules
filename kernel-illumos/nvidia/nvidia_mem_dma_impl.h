/*
 * SPDX-FileCopyrightText: Copyright 2026 Edgecast Cloud LLC.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Kept separate so the exact DDI bind and release paths can run in a host
 * test with substitute DDI calls. The includer provides nv_dma_chunk_t,
 * nv_kva_to_phys(), and the illumos DMA types.
 */
static NvBool
nv_dma_identity_bind_chunk(nv_dma_chunk_t *c, int (*waitfp)(caddr_t))
{
    ddi_dma_cookie_t cookie;
    ddi_dma_impl_t *impl;
    size_t covered = 0;
    uint_t ccount, i;

    if (c->len == 0 ||
        ddi_dma_addr_bind_handle(c->dma_handle, NULL, c->kva, c->len,
            DDI_DMA_RDWR | DDI_DMA_CONSISTENT, waitfp, NULL,
            &cookie, &ccount) != DDI_DMA_MAPPED)
        return NV_FALSE;

    c->bound = NV_TRUE;
    impl = (ddi_dma_impl_t *)c->dma_handle;
    if ((impl->dmai_rflags & DMP_NOSYNC) != DMP_NOSYNC || ccount == 0)
        goto reject;

    for (i = 0; i < ccount; i++)
    {
        size_t off;

        if (i != 0)
            ddi_dma_nextcookie(c->dma_handle, &cookie);

        if (cookie.dmac_size == 0 ||
            (cookie.dmac_laddress & (MMU_PAGESIZE - 1)) != 0 ||
            (cookie.dmac_size & (MMU_PAGESIZE - 1)) != 0 ||
            cookie.dmac_size > c->len - covered ||
            cookie.dmac_laddress > UINT64_MAX - (cookie.dmac_size - 1))
            goto reject;

        for (off = 0; off < cookie.dmac_size; off += MMU_PAGESIZE)
        {
            if (cookie.dmac_laddress + off !=
                nv_kva_to_phys(c->kva + covered + off))
                goto reject;
        }
        covered += cookie.dmac_size;
    }

    if (covered == c->len)
        return NV_TRUE;

reject:
    if (ddi_dma_unbind_handle(c->dma_handle) == DDI_SUCCESS)
        c->bound = NV_FALSE;
    return NV_FALSE;
}

/* FALSE means the DMA binding still owns its pages; never free either handle. */
static NvBool
nv_dma_release_chunk(nv_dma_chunk_t *c)
{
    if (c->bound)
    {
        if (ddi_dma_unbind_handle(c->dma_handle) != DDI_SUCCESS)
            return NV_FALSE;
        c->bound = NV_FALSE;
    }

    if (c->acc_handle != NULL)
        ddi_dma_mem_free(&c->acc_handle);
    if (c->dma_handle != NULL)
        ddi_dma_free_handle(&c->dma_handle);
    return NV_TRUE;
}
