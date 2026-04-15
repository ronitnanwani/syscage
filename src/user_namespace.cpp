#include "user_namespace.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#include <sched.h>
#include <linux/limits.h>

#define USERNS_OFFSET 10000
#define USERNS_COUNT  2000

int handle_child_uid_map(pid_t child_pid, int fd) {
    int has_userns = -1;

    // Wait for the child to tell us whether it entered a user namespace
    if (read(fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
        fprintf(stderr, "couldn't read from child!\n");
        return -1;
    }

    if (has_userns) {
        // Write uid_map and gid_map for the child process
        const char* files[] = {"uid_map", "gid_map", nullptr};

        for (const char** file = files; *file; file++) {
            char path[PATH_MAX] = {0};
            if (snprintf(path, sizeof(path), "/proc/%d/%s",
                         child_pid, *file) >= static_cast<int>(sizeof(path))) {
                fprintf(stderr, "snprintf too big? %m\n");
                return -1;
            }

            fprintf(stderr, "writing %s...", path);
            int uid_map = open(path, O_WRONLY);
            if (uid_map == -1) {
                fprintf(stderr, "open failed: %m\n");
                return -1;
            }

            // Map uid 0 inside the container to USERNS_OFFSET on the host
            if (dprintf(uid_map, "0 %d %d\n", USERNS_OFFSET, USERNS_COUNT) == -1) {
                fprintf(stderr, "dprintf failed: %m\n");
                close(uid_map);
                return -1;
            }
            close(uid_map);
        }
    }

    // Signal the child to proceed
    int zero = 0;
    if (write(fd, &zero, sizeof(zero)) != sizeof(zero)) {
        fprintf(stderr, "couldn't write: %m\n");
        return -1;
    }

    return 0;
}

int setup_userns(ContainerConfig& config) {
    fprintf(stderr, "=> trying a user namespace...");

    // Attempt to enter a new user namespace
    int has_userns = !unshare(CLONE_NEWUSER);

    // Tell the parent whether we succeeded
    if (write(config.fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
        fprintf(stderr, "couldn't write: %m\n");
        return -1;
    }

    // Wait for the parent to finish setting up uid/gid maps
    int result = 0;
    if (read(config.fd, &result, sizeof(result)) != sizeof(result)) {
        fprintf(stderr, "couldn't read: %m\n");
        return -1;
    }
    if (result) {
        return -1;
    }

    if (has_userns) {
        fprintf(stderr, "done.\n");
    } else {
        fprintf(stderr, "unsupported? continuing.\n");
    }

    // Switch to the target uid/gid
    fprintf(stderr, "=> switching to uid %d / gid %d...", config.uid, config.uid);
    gid_t gid = config.uid;
    if (setgroups(1, &gid)
        || setresgid(config.uid, config.uid, config.uid)
        || setresuid(config.uid, config.uid, config.uid)) {
        fprintf(stderr, "%m\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    return 0;
}
