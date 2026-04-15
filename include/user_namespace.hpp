#ifndef USER_NAMESPACE_HPP
#define USER_NAMESPACE_HPP

#include "config.hpp"
#include <sys/types.h>

// Child-side: attempt to unshare into a user namespace, notify parent,
// wait for parent to write uid/gid maps, then setgroups/setresgid/setresuid.
int setup_userns(ContainerConfig& config);

// Parent-side: read from socket whether child has a user namespace,
// write uid_map and gid_map for the child, then signal child to proceed.
int handle_child_uid_map(pid_t child_pid, int fd);

#endif // USER_NAMESPACE_HPP
