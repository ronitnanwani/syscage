#include "config.hpp"
#include "container.hpp"
#include "hostname.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/utsname.h>

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s -u <uid> -m <rootfs_dir> -c <command> [args...]\n", prog);
}

int main(int argc, char** argv) {
    ContainerConfig config;
    int option = 0;
    int last_optind = 0;
    int err = 0;

    while ((option = getopt(argc, argv, "c:m:u:")) != -1) {
        switch (option) {
            case 'c':
                config.argc = argc - last_optind - 1;
                config.command = std::string(argv[optind - 1]);
                // Collect command and its arguments
                for (int i = optind - 1; i < argc; i++) {
                    config.argv.push_back(argv[i]);
                }
                goto finish_options;
            case 'm':
                config.mount_dir = std::string(optarg);
                break;
            case 'u':
                if (sscanf(optarg, "%d", reinterpret_cast<int*>(&config.uid)) != 1) {
                    fprintf(stderr, "badly-formatted uid: %s\n", optarg);
                    print_usage(argv[0]);
                    return 1;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
        last_optind = optind;
    }

finish_options:
    if (!config.argc || config.mount_dir.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    // Validate Linux version — we blacklist capabilities/syscalls, so version matters
    fprintf(stderr, "=> validating Linux version...");
    struct utsname host = {};
    if (uname(&host)) {
        fprintf(stderr, "failed: %m\n");
        return 1;
    }
    int major = -1;
    int minor = -1;
    if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
        fprintf(stderr, "weird release format: %s\n", host.release);
        return 1;
    }
    if (major < 4 || (major == 4 && minor < 7)) {
        fprintf(stderr, "expected kernel >= 4.7: %s\n", host.release);
        return 1;
    }
    if (strcmp("x86_64", host.machine)) {
        fprintf(stderr, "expected x86_64: %s\n", host.machine);
        return 1;
    }
    fprintf(stderr, "%s on %s.\n", host.release, host.machine);

    // Generate a unique hostname for this container instance
    config.hostname = generate_hostname();

    fprintf(stderr,"argc = %d\n",config.argc);
    fprintf(stderr,"uid = %d\n",config.uid);
    fprintf(stderr,"mount_dir = %s\n",config.mount_dir.c_str());
    fprintf(stderr,"hostname = %s\n",config.hostname.c_str());
    fprintf(stderr,"command = %s\n",config.command.c_str());
    for(int i = 0; i < config.argv.size(); i++) {
        fprintf(stderr,"argv[%d] = %s\n",i,config.argv[i]);
    }

    Container container(config);
    err = container.start();

    return err;
}
