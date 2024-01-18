#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string_view>
#include <openssl/md5.h>
#include <stdio.h>
#include <string>
#include "cJSON.h"
#include "util.h"

bool is_raspi() {
  FILE *f;
  char buffer[256];
  bool isPi = false;

  f = fopen("/proc/cpuinfo", "r");
  if (f == NULL) {
    return 0; // Can't open the file, assuming not a Pi
  }

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "Raspberry Pi")) {
      isPi = true;
      break;
    }
  }

  fclose(f);
  return isPi;
}

void memory_usage() {
  printf("memory used: ");
  fflush(stdout);
  char command[256];
  sprintf(command, "ps -o rss= -p %d", getpid());
  system(command);
}

std::string join_path(const char* path1, const char* path2) {
  if (path1 == nullptr && path2 == nullptr) {
    return "";
  }
  if (path1 == nullptr && path2 != nullptr) {
    return path2;
  }
  if (path1 != nullptr && path2 == nullptr) {
    return path1;
  }
  //then (path1 != NULL && path2 != NULL)
  if (strlen(path2) == 1 && path2[0] == '/') {
    return path1;
  }

  //then path1!=NULL && path2.len > 0
  bool end_with_slash = false;
  int len = strlen(path1);
  if (path1[len-1] == '/') {
    end_with_slash = true;
  }
  bool start_with_slash = false;
  if (path2[0] == '/') {
    start_with_slash = true;
  }

  std::string dst = path1;
  if (end_with_slash && start_with_slash) {
    // path1/+/path2
    dst += &path2[1];
    return dst;
  }
  if ((end_with_slash && !start_with_slash) || (!end_with_slash && start_with_slash)) {
    // path1/ + path2
    // path1 + /path2
    dst += path2;
    return dst;
  }
  //then path1 + path2
  dst += "/";
  dst += path2;
  return dst;
}

std::string generate_random_string(size_t size) {
  std::string badge;
  char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand(time(NULL));
  
  int i;
  for (i = 0; i < size; ++i) {
    int index = rand() % (sizeof(characters) - 1);
    badge.push_back(characters[index]);
  }
  
  return badge;
}

std::string compute_md5(std::string_view filepath) {
  unsigned char c[MD5_DIGEST_LENGTH];
  int i;
  FILE *inFile = fopen(filepath.data(), "rb");
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];

  if (inFile == NULL) {
    printf("%s can't be opened.\n", filepath.data());
    return "";
  }

  MD5_Init(&mdContext);
  while ((bytes = fread(data, 1, 1024, inFile)) != 0)
    MD5_Update(&mdContext, data, bytes);
  MD5_Final(c,&mdContext);

  std::string dst;
  dst.reserve(32);
  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&dst[i*2], "%02x", (unsigned int)c[i]);
  }
  dst[32] = '\0';
  
  fclose(inFile);

  return dst;
}

int find_char_last(char* str, char c) {
	char *p = str;
	char* last = NULL;
	while((p = strchr(p, c))){
		last = p;
		p++;
	}
  if (last == NULL) {
    return -1;
  }
  return (unsigned)(last - str);
}

std::string cJSON_get_string(cJSON* json, char* key) {
  return cJSON_GetObjectItem(json, key)->valuestring;
}

void cJSON_copy(std::string_view dest, cJSON* json, char* key) {
 char* item = cJSON_GetObjectItem(json, key)->valuestring;

 dest = item;
}

//copy only the dest amount of src to dest
void ncpy(char* dest, char* src, size_t size) {
  strncpy(dest, src, size);
  dest[size] = '\0';;
}

void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  ncpy(dest, item, size);
}

void cJSON_cpy(cJSON* json, char* key, char* dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  strcpy(dest, item);
}

void cJSON_malloc_cpy(cJSON* json, char* key, char** dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  int len = strlen(item);
  *dest = (char*)malloc(sizeof(char)*(len + 1));
  strcpy(*dest, item);
}

char* read_file_malloc(std::string_view filename) {
  FILE* fp = fopen(filename.data(), "rb");

  if (fp == NULL) {
    printf("failed to read from file = %s\n", filename.data());
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    printf("failed to allocate memory for buffer in read_file\n");
    fclose(fp);
    return NULL;
  }

  size_t count = fread((void*)buffer, 1, file_size, fp);
  if (count != file_size) {
    printf("Error reading a file in read_file_malloc\n");
    free(buffer);
    fclose(fp);
    return NULL;
  }

  buffer[file_size] = '\0';
  fclose(fp);
  return buffer;
}

cJSON* read_file_malloc_json(std::string_view file) {
  char* output = read_file_malloc(file);
  cJSON* json  = cJSON_Parse(output);

  free(output); //TODO safe? probably yes
  return json;
}
