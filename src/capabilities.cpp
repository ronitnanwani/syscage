#include "capabilities.hpp"

#include <cstdio>
#include <array>
#include <linux/capability.h>
#include <sys/capability.h>
#include <sys/prctl.h>

int drop_capabilities() {
    fprintf(stderr, "=> dropping capabilities...");

    std::array<int, 20> drop_caps = {
        CAP_AUDIT_CONTROL,
        CAP_AUDIT_READ,
        CAP_AUDIT_WRITE,
        CAP_BLOCK_SUSPEND,
        CAP_DAC_READ_SEARCH,
        CAP_FSETID,
        CAP_IPC_LOCK,
        CAP_MAC_ADMIN,
        CAP_MAC_OVERRIDE,
        CAP_MKNOD,
        CAP_SETFCAP,
        CAP_SYSLOG,
        CAP_SYS_ADMIN,
        CAP_SYS_BOOT,
        CAP_SYS_MODULE,
        CAP_SYS_NICE,
        CAP_SYS_RAWIO,
        CAP_SYS_RESOURCE,
        CAP_SYS_TIME,
        CAP_WAKE_ALARM,
    };

    // 1. Drop from the bounding set — limits capabilities gained on execve
    fprintf(stderr, "bounding...");
    for (size_t i = 0; i < drop_caps.size(); i++) {
        if (prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0)) {
            fprintf(stderr, "prctl failed: %m\n");
            return 1;
        }
    }

    // 2. Clear from the inheritable set — also clears the ambient set
    fprintf(stderr, "inheritable...");
    cap_t caps = nullptr;
    if (!(caps = cap_get_proc())
        || cap_set_flag(caps, CAP_INHERITABLE,
                        static_cast<int>(drop_caps.size()),
                        reinterpret_cast<cap_value_t*>(drop_caps.data()),
                        CAP_CLEAR)
        || cap_set_proc(caps)) {
        fprintf(stderr, "failed: %m\n");
        if (caps) cap_free(caps);
        return 1;
    }
    cap_free(caps);

    fprintf(stderr, "done.\n");
    return 0;
}
