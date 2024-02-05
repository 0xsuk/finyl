#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string_view>
#include <openssl/md5.h>
#include <stdio.h>
#include <string>
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

  char dst[33];
  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&dst[i*2], "%02x", (unsigned int)c[i]);
  }
  dst[32] = '\0';
  
  fclose(inFile);

  return dst;
}

int find_char_last(const char* str, char c) {
	const char *p = str;
	const char* last = NULL;
	while((p = strchr(p, c))){
		last = p;
		p++;
	}
  if (last == NULL) {
    return -1;
  }
  return (unsigned)(last - str);
}

bool is_wav(const char* filename) {
  int dot = find_char_last(filename, '.');
  return strncmp(&filename[dot+1], "wav", 3) == 0;
}

void print_err(const error& err) {
  printf("err ");
  if (err.type) printf("type = %d", (int)err.type.value());
  printf(":%s\n", err.message.data());

}

error close_command(FILE* fp) {
  char error_out[10000];
  fread(error_out, 1, sizeof(error_out), fp);
  int status = pclose(fp); //TODO: takes 300,000 micsec (0.3 sec)
  if (status == -1) {
    return error("failed to close the command stream", ERR::CANT_CLOSE_COMMAND);
  }
  status = WEXITSTATUS(status);
  if (status == 1) {
    return error(std::move(error_out), ERR::COMMAND_FAILED);
  }
  return noerror;
}
