/* SPDX-License-Identifier: MIT */
#ifndef _NVIDIA_UMEM_REGISTRY_IMPL_H_
#define _NVIDIA_UMEM_REGISTRY_IMPL_H_

/*
 * Small state machine shared by the illumos pin registry and its focused
 * tests. All transitions are made while holding the registry mutex.
 */
typedef struct nv_umem_pin_state {
    unsigned int imports;
    unsigned int pending;
    unsigned int guards;
    unsigned int revoking;
    unsigned int unlocked;
} nv_umem_pin_state_t;

/* RM stores the allocation wrapper, while os_lock_user_pages returns pin. */
static inline int
nv_umem_pin_matches(const void *pin, const void *alloc_private,
    const void *candidate)
{
    return candidate != 0 &&
        (candidate == pin || candidate == alloc_private);
}

static inline void
nv_umem_pin_state_init(nv_umem_pin_state_t *pin)
{
    pin->imports = 0;
    pin->pending = 0;
    pin->guards = 0;
    pin->revoking = 0;
    pin->unlocked = 0;
}

static inline int
nv_umem_pin_reserve(nv_umem_pin_state_t *pin)
{
    if (pin->revoking || pin->unlocked)
        return 0;
    pin->imports++;
    pin->pending++;
    return 1;
}

static inline void
nv_umem_pin_publish(nv_umem_pin_state_t *pin)
{
    --pin->pending;
}

static inline void
nv_umem_pin_cancel(nv_umem_pin_state_t *pin, int pending)
{
    if (pending)
        --pin->pending;
    --pin->imports;
}

static inline void
nv_umem_pin_hardware_complete(nv_umem_pin_state_t *pin)
{
    --pin->imports;
}

static inline void
nv_umem_pin_hardware_complete_pending(nv_umem_pin_state_t *pin)
{
    --pin->pending;
    --pin->imports;
}

static inline void
nv_umem_pin_guard_acquired(nv_umem_pin_state_t *pin)
{
    pin->guards++;
}

static inline void
nv_umem_pin_guard_released(nv_umem_pin_state_t *pin)
{
    --pin->guards;
}

static inline void
nv_umem_pin_begin_revoke(nv_umem_pin_state_t *pin)
{
    pin->revoking = 1;
}

static inline int
nv_umem_pin_can_unlock(const nv_umem_pin_state_t *pin)
{
    return pin->imports == 0 && pin->guards == 0;
}

static inline void
nv_umem_pin_mark_unlocked(nv_umem_pin_state_t *pin)
{
    pin->unlocked = 1;
}

#endif
