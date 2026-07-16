/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Test that set_robust_list()/get_robust_list() are emulated rather than
 * failing with ENOSYS. glibc tolerates ENOSYS here, but some programs (e.g.
 * Steam's steamclient.so) call set_robust_list directly and abort if it does
 * not succeed. See linux-user/syscall.c.
 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>

/*
 * We only need a buffer of the kernel's expected size (three native words);
 * the exact field layout is irrelevant to the syscall's accept/echo behaviour.
 */
struct rl_head {
    void *list;
    long futex_offset;
    void *list_op_pending;
};

int main(void)
{
    struct rl_head head = { &head.list, 0, NULL };
    void *got_head = NULL;
    size_t got_len = 0;
    long ret;

    /* Registering a list of the native head size must succeed. */
    ret = syscall(__NR_set_robust_list, &head, sizeof(head));
    assert(ret == 0);

    /* get_robust_list(pid=0 -> self) must echo back exactly what we set. */
    ret = syscall(__NR_get_robust_list, 0, &got_head, &got_len);
    assert(ret == 0);
    assert(got_head == &head);
    assert(got_len == sizeof(head));

    /* The kernel rejects a mismatched head size with EINVAL. */
    ret = syscall(__NR_set_robust_list, &head, sizeof(head) + 1);
    assert(ret == -1);
    assert(errno == EINVAL);

    return 0;
}
