#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "../../nvidia/nvidia_umem_registry_impl.h"

static void
test_pending_publication_and_revoke(void)
{
    nv_umem_pin_state_t pin = {0};

    nv_umem_pin_state_init(&pin);
    assert(nv_umem_pin_reserve(&pin));
    assert(pin.pending == 1);
    nv_umem_pin_begin_revoke(&pin);
    assert(!nv_umem_pin_reserve(&pin));
    assert(!nv_umem_pin_can_unlock(&pin));
    nv_umem_pin_publish(&pin);
    assert(pin.pending == 0 && pin.imports == 1);
    assert(!nv_umem_pin_can_unlock(&pin));
    nv_umem_pin_cancel(&pin, 0);
    assert(nv_umem_pin_can_unlock(&pin));
}

static void
test_normal_release_waits_for_pending_owner(void)
{
    nv_umem_pin_state_t pin = {0};

    nv_umem_pin_state_init(&pin);
    assert(nv_umem_pin_reserve(&pin));
    assert(!nv_umem_pin_can_unlock(&pin));
    nv_umem_pin_cancel(&pin, 1);
    assert(nv_umem_pin_can_unlock(&pin));
    nv_umem_pin_mark_unlocked(&pin);
    assert(!nv_umem_pin_reserve(&pin));
}

static void
test_failed_revoke_preserves_import(void)
{
    nv_umem_pin_state_t pin = {0};

    nv_umem_pin_state_init(&pin);
    assert(nv_umem_pin_reserve(&pin));
    nv_umem_pin_publish(&pin);
    nv_umem_pin_begin_revoke(&pin);
    assert(pin.imports == 1);
    assert(!nv_umem_pin_can_unlock(&pin));
}

static void
test_hardware_complete_retains_guard_until_final_release(void)
{
    nv_umem_pin_state_t pin = {0};

    nv_umem_pin_state_init(&pin);
    assert(nv_umem_pin_reserve(&pin));
    nv_umem_pin_guard_acquired(&pin);
    nv_umem_pin_publish(&pin);
    nv_umem_pin_begin_revoke(&pin);
    nv_umem_pin_hardware_complete(&pin);
    assert(pin.imports == 0);
    assert(!nv_umem_pin_can_unlock(&pin));
    /* A failed RM guard drop leaves the pin held and owner discoverable. */
    assert(pin.guards == 1);
    nv_umem_pin_guard_released(&pin);
    assert(nv_umem_pin_can_unlock(&pin));
}

typedef struct race_context {
    pthread_mutex_t lock;
    pthread_cond_t cv;
    nv_umem_pin_state_t pin;
    int callback_claimed;
    int allow_completion;
    int hardware_done;
} race_context_t;

static void *
callback_worker(void *arg)
{
    race_context_t *ctx = arg;

    pthread_mutex_lock(&ctx->lock);
    nv_umem_pin_begin_revoke(&ctx->pin);
    ctx->callback_claimed = 1;
    pthread_cond_broadcast(&ctx->cv);
    while (!ctx->allow_completion)
        pthread_cond_wait(&ctx->cv, &ctx->lock);
    nv_umem_pin_hardware_complete(&ctx->pin);
    ctx->hardware_done = 1;
    pthread_cond_broadcast(&ctx->cv);
    pthread_mutex_unlock(&ctx->lock);
    return NULL;
}

static void
test_callback_and_guard_drop_interleave(void)
{
    race_context_t ctx = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .cv = PTHREAD_COND_INITIALIZER,
    };
    pthread_t callback;

    nv_umem_pin_state_init(&ctx.pin);
    assert(nv_umem_pin_reserve(&ctx.pin));
    nv_umem_pin_publish(&ctx.pin);
    nv_umem_pin_guard_acquired(&ctx.pin);
    assert(pthread_create(&callback, NULL, callback_worker, &ctx) == 0);

    pthread_mutex_lock(&ctx.lock);
    while (!ctx.callback_claimed)
        pthread_cond_wait(&ctx.cv, &ctx.lock);
    assert(!nv_umem_pin_reserve(&ctx.pin));
    assert(!nv_umem_pin_can_unlock(&ctx.pin));
    ctx.allow_completion = 1;
    pthread_cond_broadcast(&ctx.cv);
    while (!ctx.hardware_done)
        pthread_cond_wait(&ctx.cv, &ctx.lock);
    assert(!nv_umem_pin_can_unlock(&ctx.pin));
    nv_umem_pin_guard_released(&ctx.pin);
    assert(nv_umem_pin_can_unlock(&ctx.pin));
    pthread_mutex_unlock(&ctx.lock);
    assert(pthread_join(callback, NULL) == 0);
    pthread_cond_destroy(&ctx.cv);
    pthread_mutex_destroy(&ctx.lock);
}

static void
test_unpublished_duplicate_checked_unwind(void)
{
    nv_umem_pin_state_t pin = {0};

    nv_umem_pin_state_init(&pin);
    assert(nv_umem_pin_reserve(&pin));
    nv_umem_pin_guard_acquired(&pin);
    nv_umem_pin_begin_revoke(&pin);
    assert(pin.pending == 1);
    assert(!nv_umem_pin_can_unlock(&pin));
    /* Only after checked duplicate cleanup may the unpublished token finish. */
    nv_umem_pin_hardware_complete_pending(&pin);
    assert(pin.pending == 0 && pin.imports == 0);
    assert(!nv_umem_pin_can_unlock(&pin));
    nv_umem_pin_guard_released(&pin);
    assert(nv_umem_pin_can_unlock(&pin));
}

static void
test_rm_private_wrapper_identity(void)
{
    int pin;
    int rm_private;
    int unrelated;

    assert(nv_umem_pin_matches(&pin, &rm_private, &pin));
    assert(nv_umem_pin_matches(&pin, &rm_private, &rm_private));
    assert(!nv_umem_pin_matches(&pin, &rm_private, &unrelated));
    assert(!nv_umem_pin_matches(&pin, &rm_private, NULL));
    assert(!nv_umem_pin_matches(&pin, NULL, &rm_private));
}

int
main(void)
{
    test_pending_publication_and_revoke();
    test_normal_release_waits_for_pending_owner();
    test_failed_revoke_preserves_import();
    test_hardware_complete_retains_guard_until_final_release();
    test_callback_and_guard_drop_interleave();
    test_unpublished_duplicate_checked_unwind();
    test_rm_private_wrapper_identity();
    puts("nv_umem registry state: 7 tests passed");
    return 0;
}
