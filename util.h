#ifndef UTIL_H
#define UTIL_H

#include "error.h"
#include <string>
#include <memory>

template<typename T>
T max(T a, T b) {
  return a>b ? a : b;
}

template<typename T>
T min(T a, T b) {
  return a<b ? a : b;
}



bool is_raspi();
void memory_usage();
std::string join_path(const char* path1, const char* path2);

std::string generate_random_string(size_t size);

std::string compute_md5(std::string_view filepath);

int find_char_last(const char* str, char c);
bool is_wav(const char* filename);
void print_err(const error& err);

#endif
