#include <stdio.h>
#include "json.h"

int main() {
  auto t = json::tokenizer("/home/null/.finyl-output");

  if (!t.file.good()) {
    printf("not good\n");
  }
  
  for (int i = 0; i< 2; i++) {
    
  auto token = t.get();
  printf("%s %d\n", token.value.data(), token.type);
  }
}
