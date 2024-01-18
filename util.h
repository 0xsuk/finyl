#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string>
#include <memory>
#include "cJSON.h"

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
std::string join_path(char* path1, char* path2);

std::string generate_random_string(size_t size);

std::string compute_md5(std::string& filepath);

int find_char_last(char* str, char c);

void ncpy(char* dest, char* src, size_t size);
void cJSON_copy(std::string& dest, cJSON* json, char* key);
std::string cJSON_get_string(cJSON* json, char* key);
void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size);
void cJSON_cpy(cJSON* json, char* key, char* dest);
void cJSON_malloc_cpy(cJSON* json, char* key, char** dest);
char* read_file_malloc(char* filename);
cJSON* read_file_malloc_json(char* file);
#endif
