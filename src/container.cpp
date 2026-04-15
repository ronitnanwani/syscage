#include "container.hpp"
#include "mounts.hpp"
#include "user_namespace.hpp"
#include "capabilities.hpp"
#include "syscalls.hpp"
#include "resources.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#define STACK_SIZE (1024 * 1024)

Container::Container(ContainerConfig& config)
    : config_(config), child_pid_(-1), stack_(nullptr) {
    sockets_[0] = 0;
    sockets_[1] = 0;
    // Allocate memory for the child's stack
    stack_ = new char[STACK_SIZE];
}

Container::~Container() {
    delete[] stack_;
    if (sockets_[0]) close(sockets_[0]);
    if (sockets_[1]) close(sockets_[1]);
}

int Container::start() {
    // 1. Create the socket pair for parent/child synchronization
    if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets_) < 0) {
        fprintf(stderr, "socketpair failed: %m\n");
        return -1;
    }
    // Set close-on-exec on the parent's socket so it doesn't leak to the child's execve
    if (fcntl(sockets_[0], F_SETFD, FD_CLOEXEC)) {
        fprintf(stderr, "fcntl failed: %m\n");
        return -1;
    }
    config_.fd = sockets_[1];

    // 2. Set up cgroup resource limits BEFORE cloning
    if (setup_resources(config_)) {
        return -1;
    }

    // 3. Clone with all the namespace flags
    int flags = CLONE_NEWNS      // Mount namespace
              | CLONE_NEWCGROUP  // Cgroup namespace
              | CLONE_NEWPID     // PID namespace
              | CLONE_NEWIPC     // IPC namespace
              | CLONE_NEWNET     // Network namespace
              | CLONE_NEWUTS;    // UTS namespace (hostname)

    // clone() requires the TOP of the stack (grows downward on x86_64)
    child_pid_ = clone(child_entry, stack_ + STACK_SIZE,
                       flags | SIGCHLD, this);

    if (child_pid_ == -1) {
        fprintf(stderr, "=> clone failed! %m\n");
        return -1;
    }

    // Close the child's end of the socket on the parent side
    close(sockets_[1]);
    sockets_[1] = 0;

    // 4. Transition to the parent's monitoring routine
    return run_parent();
}

// Static trampoline for clone()
int Container::child_entry(void* arg) {
    Container* container = static_cast<Container*>(arg);
    return container->run_child();
}

int Container::run_child() {
    // Close the parent's end of the socket
    close(sockets_[0]);
    sockets_[0] = 0;

    // The order matters here:
    // 1. Set hostname (needs no special caps, just UTS namespace)
    // 2. Set up mounts / pivot_root (needs mount caps)
    // 3. Set up user namespace + switch uid/gid (needs parent coordination)
    // 4. Drop capabilities (restrict what root can do)
    // 5. Filter syscalls with seccomp (locks down further)
    // 6. Close the socket and execve the target program

    if (sethostname(config_.hostname.c_str(), config_.hostname.size())
        || setup_mounts(config_)
        || setup_userns(config_)
        || drop_capabilities()
        || setup_syscall_filter()) {
        close(config_.fd);
        return -1;
    }

    if (close(config_.fd)) {
        fprintf(stderr, "close failed: %m\n");
        return -1;
    }

    // Replace the current process with the target command
    config_.argv.push_back(nullptr);
    if (execve(config_.command.c_str(), config_.argv.data(), nullptr)) {
        fprintf(stderr, "execve failed! %m\n");
        return -1;
    }

    return 0; // unreachable
}

int Container::run_parent() {
    // Handle user namespace uid/gid mapping for the child
    fprintf(stderr, "=> trying a user namespace...");
    if (handle_child_uid_map(child_pid_, sockets_[0])) {
        return -1;
    }
    fprintf(stderr, "done.\n");

    // Wait for the child process to exit
    int status = 0;
    if (waitpid(child_pid_, &status, 0) == -1) {
        fprintf(stderr, "waitpid failed: %m\n");
        return -1;
    }

    // Clean up cgroup directories
    if (free_resources(config_)) {
        return -1;
    }

    return WEXITSTATUS(status);
}