#!/usr/bin/env python3
"""Exercise real devmap callbacks when the fd closes before final unmap."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "kernel-illumos/nvidia/nvidia_mmap.c"
REF_SOURCE = ROOT / "kernel-illumos/nvidia/nvidia_cb.c"


def function(source: str, name: str) -> str:
    start = source.index(name + "(")
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                if name == "nv_file_private_hold_mapping":
                    declaration = source.rfind("\nvoid\n", 0, start) + 1
                else:
                    declaration = source.rfind("static ", 0, start)
                return source[declaration:index + 1]
    raise ValueError(f"unterminated function: {name}")


source = SOURCE.read_text()
hold_mapping = function(REF_SOURCE.read_text(), "nv_file_private_hold_mapping")
callbacks = "\n\n".join(
    function(source, name)
    for name in (
        "nv_devmap_priv_create",
        "nv_devmap_priv_destroy",
        "nv_devmap_dup",
        "nv_devmap_unmap",
    )
)

harness = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

typedef int NvBool;
typedef void *devmap_cookie_t;
typedef long long offset_t;
typedef struct nv_illumos_alloc_s { int refs; } nv_illumos_alloc_t;
typedef struct nv_devmap_priv_s nv_devmap_priv_t;
typedef struct { nv_devmap_priv_t *head, *tail; } list_t;
typedef struct { int count; } ksema_t;
typedef struct { int held; } kmutex_t;
typedef struct nv_illumos_state_s { ksema_t mmap_lock; } nv_illumos_state_t;
typedef struct nv_illumos_file_private_s {
    int refcnt;
    int closing;
    int destroyed;
    kmutex_t ref_lock;
    nv_illumos_state_t *nvis;
    list_t mappings;
} nv_illumos_file_private_t;
struct nv_devmap_priv_s {
    nv_devmap_priv_t *prev, *next;
    nv_illumos_file_private_t *nvifp;
    devmap_cookie_t dhp;
    nv_illumos_alloc_t *at;
    offset_t off;
    size_t len;
    NvBool device_memory;
};

#define KM_SLEEP 0
#define ASSERT(expr) assert(expr)
static void mutex_enter(kmutex_t *mutex)
{ assert(!mutex->held); mutex->held = 1; }
static void mutex_exit(kmutex_t *mutex)
{ assert(mutex->held); mutex->held = 0; }
static void *kmem_zalloc(size_t size, int flags)
{ (void)flags; return calloc(1, size); }
static void kmem_free(void *ptr, size_t size)
{ (void)size; free(ptr); }
static void nv_alloc_hold(nv_illumos_alloc_t *at) { ++at->refs; }
static void nv_alloc_rele(nv_illumos_alloc_t *at) { assert(at->refs > 0); --at->refs; }
static void sema_p(ksema_t *sema) { assert(sema->count == 1); sema->count = 0; }
static void sema_v(ksema_t *sema) { assert(sema->count == 0); sema->count = 1; }
static void list_insert_tail(list_t *list, nv_devmap_priv_t *dp)
{
    dp->prev = list->tail;
    if (list->tail != NULL) list->tail->next = dp;
    else list->head = dp;
    list->tail = dp;
}
static void list_remove(list_t *list, nv_devmap_priv_t *dp)
{
    if (dp->prev != NULL) dp->prev->next = dp->next;
    else { assert(list->head == dp); list->head = dp->next; }
    if (dp->next != NULL) dp->next->prev = dp->prev;
    else { assert(list->tail == dp); list->tail = dp->prev; }
}
static void nv_file_private_rele(nv_illumos_file_private_t *file)
{
    assert(file->refcnt > 0);
    if (--file->refcnt == 0) {
        assert(file->closing);
        assert(file->mappings.head == NULL);
        file->destroyed = 1;
    }
}

''' + hold_mapping + "\n\n" + callbacks + r'''

static void init_file(nv_illumos_file_private_t *file, nv_illumos_state_t *state)
{
    *state = (nv_illumos_state_t){ .mmap_lock = { .count = 1 } };
    *file = (nv_illumos_file_private_t){ .refcnt = 1, .nvis = state };
}

static void close_fd(nv_illumos_file_private_t *file)
{ file->closing = 1; nv_file_private_rele(file); }

int main(void)
{
    nv_illumos_file_private_t file;
    nv_illumos_state_t state;
    nv_illumos_alloc_t allocation = { .refs = 1 };
    nv_devmap_priv_t *original, *copy, *left, *right;

    init_file(&file, &state);
    original = nv_devmap_priv_create(&file, (void *)1, &allocation, 0, 4096, 0);
    close_fd(&file);
    assert(!file.destroyed && file.refcnt == 1);
    assert(nv_devmap_dup(NULL, original, (void *)2, (void **)&copy) == 0);
    nv_devmap_priv_destroy(original);
    assert(!file.destroyed && file.refcnt == 1);
    nv_devmap_priv_destroy(copy);
    assert(file.destroyed && file.refcnt == 0 && allocation.refs == 1);

    init_file(&file, &state);
    original = nv_devmap_priv_create(&file, (void *)3, &allocation, 0, 4096, 0);
    close_fd(&file);
    nv_devmap_unmap(NULL, original, 1024, 1024,
        (void *)4, (void **)&left, (void *)5, (void **)&right);
    assert(!file.destroyed && file.refcnt == 2);
    nv_devmap_priv_destroy(left);
    assert(!file.destroyed && file.refcnt == 1);
    nv_devmap_priv_destroy(right);
    assert(file.destroyed && file.refcnt == 0 && allocation.refs == 1);
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    (path / "test.c").write_text(harness)
    subprocess.run(
        ["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
         "-Wno-unused-parameter", "-Wno-unused-function", "-o",
         str(path / "test"), str(path / "test.c")],
        check=True,
    )
    subprocess.run([str(path / "test")], check=True)

print("devmap mapping lifetime: 2 scenarios passed")
