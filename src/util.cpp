#include "util.h"
#include <sstream>
#include <iomanip>
#include <functional>
#include <sys/stat.h>

std::string to_hex(size_t x){
    std::ostringstream ss;
    ss << std::hex << x;
    return ss.str();
}

std::string simple_hash(const std::string &s){
    // НЕ криптостойкий хеш! Только демонстрация.
    std::hash<std::string> h;
    size_t v = h(s + "SALT_DEMO_v1");
    return to_hex(v);
}

bool file_exists(const std::string &path){
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}
