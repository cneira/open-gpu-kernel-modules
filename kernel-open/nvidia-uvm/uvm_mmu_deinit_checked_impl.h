/* SPDX-License-Identifier: MIT */
/* Included by uvm_mmu.c and the focused failed-drain test. */
NV_STATUS uvm_page_table_range_vec_deinit_checked(
    uvm_page_table_range_vec_t *range_vec, bool *put_queued)
{
    size_t i;
    NV_STATUS status;

    if (range_vec == NULL)
        return NV_OK;
    if (put_queued == NULL)
        return NV_ERR_INVALID_ARGUMENT;

    if (range_vec->ranges != NULL) {
        if (!*put_queued) {
            for (i = 0; i < range_vec->range_count; ++i) {
                uvm_page_table_range_t *range = &range_vec->ranges[i];
                if (!range->entry_count)
                    break;
                uvm_page_tree_put_ptes_async(range_vec->tree, range);
            }
            *put_queued = true;
        }

        status = uvm_page_tree_wait(range_vec->tree);
        if (status != NV_OK)
            return status;

        uvm_kvfree(range_vec->ranges);
    }

    memset(range_vec, 0, sizeof (*range_vec));
    *put_queued = false;
    return NV_OK;
}
