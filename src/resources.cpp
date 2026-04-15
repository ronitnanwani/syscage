#include "resources.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <linux/limits.h>

// Resource limits
static constexpr const char* MEMORY_MAX = "1073741824";  // 1 GB
static constexpr const char* CPU_WEIGHT = "50";          // low weight (default 100, range 1-10000)
static constexpr const char* PIDS_MAX   = "64";          // max 64 pids
static constexpr const char* IO_WEIGHT  = "50";          // low IO priority (default 100, range 1-10000)
static constexpr int FD_COUNT           = 64;            // max file descriptors

struct CgroupSetting {
    std::string name;
    std::string value;
};

// Helper: write a string to a file
static int write_file(const std::string& path, const std::string& value) {
    int fd = open(path.c_str(), O_WRONLY);
    if (fd == -1) {
        fprintf(stderr, "opening %s failed: %m\n", path.c_str());
        return -1;
    }
    if (write(fd, value.c_str(), value.size()) == -1) {
        fprintf(stderr, "writing to %s failed: %m\n", path.c_str());
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int setup_resources(const ContainerConfig& config) {
    fprintf(stderr, "=> setting cgroups...");

    // Cgroups v2: single unified directory under /sys/fs/cgroup/<hostname>
    std::string cgroup_dir = "/sys/fs/cgroup/" + config.hostname;

    // Create the cgroup directory
    if (mkdir(cgroup_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR)) {
        fprintf(stderr, "mkdir %s failed: %m\n", cgroup_dir.c_str());
        return -1;
    }
    fprintf(stderr, "created %s...", cgroup_dir.c_str());

    // Enable controllers in the subtree
    // We need to enable the controllers we want to use for this cgroup
    // by writing to the parent's cgroup.subtree_control
    // (The root cgroup should already have them enabled)

    // Apply resource limits using cgroups v2 file names
    std::vector<CgroupSetting> settings = {
        {"memory.max",  MEMORY_MAX},     // Memory limit (replaces memory.limit_in_bytes)
        {"cpu.weight",  CPU_WEIGHT},     // CPU weight (replaces cpu.shares; range 1-10000, default 100)
        {"pids.max",    PIDS_MAX},       // Max PIDs
        {"io.weight",   IO_WEIGHT},      // IO weight (replaces blkio.weight; range 1-10000, default 100)
    };

    for (const auto& setting : settings) {
        std::string path = cgroup_dir + "/" + setting.name;
        // Some controllers might not be available, warn but continue
        if (write_file(path, setting.value) == -1) {
            fprintf(stderr, "(warning: %s skipped)...", setting.name.c_str());
            // Non-fatal: some controllers may not be delegated
        }
    }

    // Add current process to the cgroup
    std::string procs_path = cgroup_dir + "/cgroup.procs";
    if (write_file(procs_path, "0") == -1) {
        fprintf(stderr, "failed to add process to cgroup!\n");
        return -1;
    }

    fprintf(stderr, "done.\n");

    // Set rlimit on file descriptors
    fprintf(stderr, "=> setting rlimit...");
    struct rlimit rl = {};
    rl.rlim_max = FD_COUNT;
    rl.rlim_cur = FD_COUNT;
    if (setrlimit(RLIMIT_NOFILE, &rl)) {
        fprintf(stderr, "failed: %m\n");
        return 1;
    }
    fprintf(stderr, "done.\n");

    return 0;
}

int free_resources(const ContainerConfig& config) {
    fprintf(stderr, "=> cleaning cgroups...");

    std::string cgroup_dir = "/sys/fs/cgroup/" + config.hostname;

    // Move ourselves back to the root cgroup
    if (write_file("/sys/fs/cgroup/cgroup.procs", "0") == -1) {
        fprintf(stderr, "failed to move to root cgroup!\n");
        return -1;
    }

    // Remove the cgroup directory (should be empty now)
    if (rmdir(cgroup_dir.c_str())) {
        fprintf(stderr, "rmdir %s failed: %m\n", cgroup_dir.c_str());
        return -1;
    }

    fprintf(stderr, "done.\n");
    return 0;
}
