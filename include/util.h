#pragma once
#include <string>

std::string to_hex(size_t x);
std::string simple_hash(const std::string &s);
bool file_exists(const std::string &path);
