#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include "config.hpp"

// Set up cgroup v1 resource limits (memory, cpu, pids, blkio) and rlimit.
// Must be called BEFORE clone so the child inherits the cgroup.
int setup_resources(const ContainerConfig& config);

// Clean up cgroup directories after the container exits.
// Moves the calling process back to root tasks, then rmdirs the cgroup dirs.
int free_resources(const ContainerConfig& config);

#endif // RESOURCES_HPP
