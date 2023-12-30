#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <stdio.h>
#include "cJSON.h"
#include "util.h"

void memory_usage() {
  printf("memory used: ");
  fflush(stdout);
  char command[256];
  sprintf(command, "ps -o rss= -p %d", getpid());
  system(command);
}

void join_path(char* dst, char* path1, char* path2) {
  if (path1 == NULL && path2 == NULL) {
    return;
  }
  if (path1 == NULL && path2 != NULL) {
    strcpy(dst, path2);
    return;
  }
  if (path1 != NULL && path2 == NULL) {
    strcpy(dst, path1);
    return;
  }
  //then (path1 != NULL && path2 != NULL)
  if (strlen(path2) == 1 && path2[0] == '/') {
    strcpy(dst, path1);
    return;
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

  strcpy(dst, path1);
  if (end_with_slash && start_with_slash) {
    // path1/+/path2
    strcat(dst, &path2[1]);
    return;
  }
  if ((end_with_slash && !start_with_slash) || (!end_with_slash && start_with_slash)) {
    // path1/ + path2
    // path1 + /path2
    strcat(dst, path2);
    return;
  }
  //then path1 + path2
  strcat(dst, "/");
  strcat(dst, path2);
}

void generateRandomString(char* badge, size_t size) {
  char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand(time(NULL));
  
  int i;
  for (i = 0; i < size - 1; ++i) {
    int index = rand() % (sizeof(characters) - 1);
    badge[i] = characters[index];
  }
  badge[size - 1] = '\0';
}

void compute_md5(const char *filepath, char *dst) {
  unsigned char c[MD5_DIGEST_LENGTH];
  int i;
  FILE *inFile = fopen(filepath, "rb");
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];

  if (inFile == NULL) {
    printf("%s can't be opened.\n", filepath);
    return;
  }

  MD5_Init(&mdContext);
  while ((bytes = fread(data, 1, 1024, inFile)) != 0)
    MD5_Update(&mdContext, data, bytes);
  MD5_Final(c,&mdContext);

  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&dst[i*2], "%02x", (unsigned int)c[i]);
  }
  dst[32] = '\0';
  
  fclose(inFile);
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

char* read_file_malloc(char* filename) {
  FILE* fp = fopen(filename, "rb");

  if (fp == NULL) {
    printf("failed to read from file = %s\n", filename);
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

cJSON* read_file_malloc_json(char* file) {
  char* output = read_file_malloc(file);
  cJSON* json  = cJSON_Parse(output);

  free(output); //TODO safe? probably yes
  return json;
}
