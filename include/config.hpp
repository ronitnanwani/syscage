#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <sys/types.h>

struct ContainerConfig {
    int argc = 0;
    uid_t uid = -1;
    std::string mount_dir;
    std::string hostname;
    std::string command;
    std::vector<char*> argv;
    int fd = -1;
};

#endif