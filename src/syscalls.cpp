#include "syscalls.hpp"

#include <cstdio>
#include <cerrno>
#include <seccomp.h>
#include <sys/stat.h>
#include <linux/sched.h>

// ioctl constant for injecting characters into the terminal input queue.
// Not always defined in user headers.
#ifndef TIOCSTI
#define TIOCSTI 0x5412
#endif

#define SCMP_FAIL SCMP_ACT_ERRNO(EPERM)

int setup_syscall_filter() {
    scmp_filter_ctx ctx = nullptr;
    fprintf(stderr, "=> filtering syscalls...");

    if (!(ctx = seccomp_init(SCMP_ACT_ALLOW))

        // Prevent creating setuid/setgid executables
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
               SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
               SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
               SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
               SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
               SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
               SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID))

        // Prevent new user namespaces (stops privilege escalation)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(unshare), 1,
               SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER))
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(clone), 1,
               SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER))

        // Prevent TIOCSTI — inject chars into terminal input
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ioctl), 1,
               SCMP_A1(SCMP_CMP_MASKED_EQ, TIOCSTI, TIOCSTI))

        // Kernel keyring is not namespaced
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(keyctl), 0)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(add_key), 0)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(request_key), 0)

        // ptrace can break seccomp on older kernels
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0)

        // NUMA calls can DOS other applications
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(mbind), 0)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(migrate_pages), 0)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(move_pages), 0)
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(set_mempolicy), 0)

        // userfaultfd can be used in kernel exploits
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(userfaultfd), 0)

        // perf_event_open can leak kernel addresses
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(perf_event_open), 0)

        // Prevent setuid/setcap binaries from gaining extra privileges
        || seccomp_attr_set(ctx, SCMP_FLTATR_CTL_NNP, 0)

        // Apply the filter
        || seccomp_load(ctx)) {

        if (ctx) seccomp_release(ctx);
        fprintf(stderr, "failed: %m\n");
        return 1;
    }

    seccomp_release(ctx);
    fprintf(stderr, "done.\n");
    return 0;
}
