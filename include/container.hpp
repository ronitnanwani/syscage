#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include "config.hpp"
#include <sys/types.h>

class Container {
public:
    // Constructor takes a reference to the parsed configuration
    explicit Container(ContainerConfig& config);

    // Destructor to ensure resources (like the child stack and sockets) are cleaned up
    ~Container();

    // The main entry point to launch the container
    int start();

private:
    ContainerConfig& config_;

    // The Process ID of the cloned child
    pid_t child_pid_;

    // A pair of connected sockets used for synchronization between parent and child.
    // The child needs to wait for the parent to set up User IDs before continuing.
    int sockets_[2];

    // Pointer to the memory allocated for the child process's stack
    char* stack_;

    // The entry function passed to clone().
    // It MUST be static so it has a standard C function signature: int (*)(void*)
    static int child_entry(void* arg);

    // The actual sequence of operations the child process runs (called by child_entry)
    int run_child();

    // The sequence of operations the parent process runs to monitor and configure the child
    int run_parent();
};

#endif // CONTAINER_HPP