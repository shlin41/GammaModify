#pragma once

#include <stdbool.h>
#include "atomic.h"
#include "futex.h"
#include "spinlock.h"

typedef struct {
    atomic int state;		// state only has b00, b01, b11. There will be no b10
} mutex_t;

enum {
    MUTEX_LOCKED = 1 << 0,	// b01
    MUTEX_SLEEPING = 1 << 1,	// b10
};

#define MUTEX_INITIALIZER \
    {                     \
        .state = 0        \
    }

static inline int mutex_init(mutex_t *mutex, void *dummy)
{
    atomic_init(&mutex->state, 0);
    return 0;
}

static inline int mutex_destroy(mutex_t *mutex)
{
    return 0;
}

static int mutex_trylock(mutex_t *mutex)
{
    int state = load(&mutex->state, relaxed);
    if (state & MUTEX_LOCKED)
        return -1;

    state = fetch_or(&mutex->state, MUTEX_LOCKED, relaxed);
    if (state & MUTEX_LOCKED)
        return -1;

    thread_fence(&mutex->state, acquire);		// read memory barrier
    return 0;
}

static inline int mutex_lock(mutex_t *mutex)
{
#define MUTEX_SPINS 128
    for (int i = 0; i < MUTEX_SPINS; ++i) {
        if (mutex_trylock(mutex) == 0)
            return 0;
        spin_hint();
    }

    int state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);

    while (state & MUTEX_LOCKED) {
        futex_wait(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING);
        state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);
    }

    thread_fence(&mutex->state, acquire);		// read memory barrier
    return 0;
}

static inline int mutex_unlock(mutex_t *mutex)
{
    int state = exchange(&mutex->state, 0, release);	// write memory barrier
    if (state & MUTEX_SLEEPING)
        futex_wake(&mutex->state, 1);

    return 0;
}

