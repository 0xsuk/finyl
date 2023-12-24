#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include "cJSON.h"

void join_path(char* dst, char* path1, char* path2);

void generateRandomString(char* badge, size_t size);

void compute_md5(const char *filepath, char *dst);

int find_char_last(char* str, char c);

void ncpy(char* dest, char* src, size_t size);
void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size);
void cJSON_cpy(cJSON* json, char* key, char* dest);
void cJSON_malloc_cpy(cJSON* json, char* key, char** dest);
char* read_file_malloc(char* filename);
cJSON* read_file_malloc_json(char* file);
#endif
