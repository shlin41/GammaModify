#pragma once

#include <limits.h>
#include <stddef.h>
#include "atomic.h"
#include "futex.h"
#include "mutex.h"
#include "spinlock.h"

typedef struct {
    atomic int seq;
} cond_t;

static inline int cond_init(cond_t *cond, void *dummy)
{
    atomic_init(&cond->seq, 0);
    return 0;
}

static inline int cond_destroy(cond_t *cond)
{
    return 0;
}

static inline int cond_wait(cond_t *cond, mutex_t *mutex)
{
    int seq = load(&cond->seq, relaxed);

    mutex_unlock(mutex);

#define COND_SPINS 128
    for (int i = 0; i < COND_SPINS; ++i) {
        if (load(&cond->seq, relaxed) != seq) {
            mutex_lock(mutex);
            return 0;
        }
        spin_hint();
    }

    futex_wait(&cond->seq, seq);

    mutex_lock(mutex);

    fetch_or(&mutex->state, MUTEX_SLEEPING, relaxed);

    return 0;
}

static inline int cond_signal(cond_t *cond)
{
    fetch_add(&cond->seq, 1, relaxed);
    futex_wake(&cond->seq, 1);

    return 0;
}

static inline int cond_broadcast(cond_t *cond, mutex_t *mutex)
{
    fetch_add(&cond->seq, 1, relaxed);
    futex_requeue(&cond->seq, 1, &mutex->state);

    return 0;
}

