#ifndef MOUNTS_HPP
#define MOUNTS_HPP

#include "config.hpp"

// Set up filesystem isolation for the container:
// 1. Remount everything with MS_PRIVATE
// 2. Bind-mount the user's rootfs into a temp directory
// 3. pivot_root to swap the root
// 4. Unmount and remove the old root
int setup_mounts(const ContainerConfig& config);

#endif // MOUNTS_HPP
