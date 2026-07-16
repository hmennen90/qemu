/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * End-to-end test for robust-futex cleanup at thread exit. A thread locks a
 * PTHREAD_MUTEX_ROBUST mutex and dies without unlocking it; the next locker
 * must get EOWNERDEAD rather than blocking forever. This exercises
 * target_exit_robust_list() in linux-user/syscall.c: without it the guest's
 * robust list is never walked on exit, the FUTEX_OWNER_DIED bit is never set,
 * and this test hangs.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>

static pthread_mutex_t mutex;

static void *locker(void *arg)
{
    /* Lock and return holding the mutex: the thread exits without unlocking. */
    int rc = pthread_mutex_lock(&mutex);
    assert(rc == 0);
    return NULL;
}

int main(void)
{
    pthread_mutexattr_t attr;
    pthread_t thread;
    int rc;

    rc = pthread_mutexattr_init(&attr);
    assert(rc == 0);
    rc = pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    assert(rc == 0);
    rc = pthread_mutex_init(&mutex, &attr);
    assert(rc == 0);

    rc = pthread_create(&thread, NULL, locker, NULL);
    assert(rc == 0);
    /* Join guarantees the owner has exited (and its robust list was walked). */
    rc = pthread_join(thread, NULL);
    assert(rc == 0);

    /* The owner died holding the mutex: robust mutexes report EOWNERDEAD. */
    rc = pthread_mutex_lock(&mutex);
    assert(rc == EOWNERDEAD);

    rc = pthread_mutex_consistent(&mutex);
    assert(rc == 0);
    rc = pthread_mutex_unlock(&mutex);
    assert(rc == 0);

    return 0;
}
