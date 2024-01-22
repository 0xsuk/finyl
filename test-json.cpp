#include <stdio.h>
#include "json.h"

void test_tokenizer() {
  auto t = json::tokenizer("/home/null/.finyl-output");

  if (!t.file.good()) {
    printf("not good\n");
  }
  
  while (true) {
    auto token = t.get();
    printf("%s %d\n", token.value.data(), (int)token.type);
    if (token.type == json::token_type::END) {
      break;
    }
    if (token.type == json::token_type::ERR) {
      break;
    }
  }
  
}


int main() {
  test_tokenizer();
}
