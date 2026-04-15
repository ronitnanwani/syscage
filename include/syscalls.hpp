#ifndef SYSCALLS_HPP
#define SYSCALLS_HPP

// Set up seccomp-BPF syscall filtering to block dangerous system calls.
// Uses a blacklist approach: allow everything by default, deny specific calls.
int setup_syscall_filter();

#endif // SYSCALLS_HPP
