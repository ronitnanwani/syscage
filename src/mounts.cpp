#include "mounts.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

// glibc doesn't provide a wrapper for pivot_root; use syscall directly.
static int pivot_root(const char *new_root, const char *put_old) {
    return static_cast<int>(syscall(SYS_pivot_root, new_root, put_old));
}

int setup_mounts(const ContainerConfig& config) {
    // 1. Remount everything with MS_PRIVATE so bind mounts are invisible outside
    fprintf(stderr, "=> remounting everything with MS_PRIVATE...");
    if (mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr)) {
        fprintf(stderr, "failed! %m\n");
        return -1;
    }
    fprintf(stderr, "remounted.\n");

    // 2. Create a temp directory and bind-mount the user's rootfs there
    fprintf(stderr, "=> making a temp directory and a bind mount there...");
    char mount_dir[] = "/tmp/tmp.XXXXXX";
    if (!mkdtemp(mount_dir)) {
        fprintf(stderr, "failed making a directory!\n");
        return -1;
    }

    if (mount(config.mount_dir.c_str(), mount_dir, nullptr,
              MS_BIND | MS_PRIVATE, nullptr)) {
        fprintf(stderr, "bind mount failed!\n");
        return -1;
    }

    // 3. Create an inner temp directory for the old root
    char inner_mount_dir[] = "/tmp/tmp.XXXXXX/oldroot.XXXXXX";
    memcpy(inner_mount_dir, mount_dir, sizeof(mount_dir) - 1);
    if (!mkdtemp(inner_mount_dir)) {
        fprintf(stderr, "failed making the inner directory!\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    // 4. pivot_root: swap the new root and old root
    fprintf(stderr, "=> pivoting root...");
    if (pivot_root(mount_dir, inner_mount_dir)) {
        fprintf(stderr, "failed!\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    // 5. Construct the path to the old root inside the new root
    //    inner_mount_dir was like "/tmp/tmp.XXXXXX/oldroot.YYYYYY"
    //    After pivot_root, the old root is at "/oldroot.YYYYYY"
    char *old_root_dir = basename(inner_mount_dir);
    char old_root[sizeof(inner_mount_dir) + 1] = {"/"};
    strcpy(&old_root[1], old_root_dir);

    // 6. Unmount the old root and remove the directory
    fprintf(stderr, "=> unmounting %s...", old_root);
    if (chdir("/")) {
        fprintf(stderr, "chdir failed! %m\n");
        return -1;
    }
    if (umount2(old_root, MNT_DETACH)) {
        fprintf(stderr, "umount failed! %m\n");
        return -1;
    }
    if (rmdir(old_root)) {
        fprintf(stderr, "rmdir failed! %m\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    // 7. Mount /proc so tools like ps, top, etc. work inside the container
    fprintf(stderr, "=> mounting /proc...");
    mkdir("/proc", 0555);
    if (mount("proc", "/proc", "proc", 0, nullptr)) {
        fprintf(stderr, "failed! %m\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    return 0;
}
