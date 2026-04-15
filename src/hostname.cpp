#include "hostname.hpp"
#include <cstdio>
#include <ctime>
#include <array>

std::string generate_hostname() {
    static const std::array<const char*, 4> suits = {
        "swords", "wands", "pentacles", "cups"
    };
    static const std::array<const char*, 14> minor = {
        "ace", "two", "three", "four", "five", "six", "seven",
        "eight", "nine", "ten", "page", "knight", "queen", "king"
    };
    static const std::array<const char*, 22> major = {
        "fool", "magician", "high-priestess", "empress", "emperor",
        "hierophant", "lovers", "chariot", "strength", "hermit",
        "wheel", "justice", "hanged-man", "death", "temperance",
        "devil", "tower", "star", "moon", "sun", "judgment", "world"
    };

    struct timespec now = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &now);

    char buff[256] = {0};
    size_t ix = static_cast<size_t>(now.tv_nsec) % 78;

    if (ix < major.size()) {
        snprintf(buff, sizeof(buff), "%05lx-%s",
                 static_cast<long>(now.tv_sec), major[ix]);
    } else {
        ix -= major.size();
        snprintf(buff, sizeof(buff), "%05lx-%s-of-%s",
                 static_cast<long>(now.tv_sec),
                 minor[ix % minor.size()],
                 suits[ix / minor.size()]);
    }

    return std::string(buff);
}
