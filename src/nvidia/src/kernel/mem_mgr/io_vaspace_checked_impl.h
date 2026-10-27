/* SPDX-License-Identifier: MIT */
/* Included by io_vaspace.c and by the focused release-failure test. */
NV_STATUS
iovaspaceReleaseMappingChecked(OBJIOVASPACE *pIOVAS,
                                PIOVAMAPPING pIovaMapping)
{
    NV_STATUS status;

    if (pIOVAS == NULL || pIovaMapping == NULL)
        return NV_ERR_INVALID_ARGUMENT;

    /* This scoped path cannot revoke aliases or child mappings. */
    if (pIovaMapping->refcount != 1 ||
        pIovaMapping->link.pChildren != NULL ||
        memdescIsSubMemoryMemDesc(pIovaMapping->pPhysMemDesc))
        return NV_ERR_NOT_SUPPORTED;

    status = osIovaUnmapChecked(pIovaMapping);
    if (status != NV_OK)
        return status;

    memdescRemoveIommuMap(pIovaMapping->pPhysMemDesc, pIovaMapping);
    portMemFree(pIovaMapping);
    --pIOVAS->mappingCount;
    return NV_OK;
}
