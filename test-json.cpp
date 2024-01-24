#include <stdio.h>
#include "json.h"

std::string filename = "/home/null/.finyl-output";

void test_tokenizer() {
  auto tokenizer = json::tokenizer(filename);

  if (!tokenizer.file.good()) {
    printf("not good\n");
  }
  
  while (true) {
    auto t = tokenizer.get();
    printf("%s %d\n", t.value.data(), (int)t.type);
    if (t.type == json::TOKEN::END) {
      printf("end of file\n");
      break;
    }
    if (t.type == json::TOKEN::ERR) {
      break;
    }
  }
}

void print_node(const json::node& n);

void print_object(const json::node& n) {
  const auto& o = std::get<json::object>(n.vals);
  
  for (auto& [key, node] : o) {
    print_node(*node);
    printf("and key is %s\n", key.data());
  }
}

void print_array(const json::node& n) {
  const auto& arr = std::get<json::array>(n.vals);
  
  printf("printing array\n\t");
  for (auto& elem : arr) {
    print_node(*elem);
  }
}

void print_string(const json::node& n) {
  const auto& s = std::get<std::string>(n.vals);

  printf("string is %s\n", s.data());
}

void print_number(const json::node& n) {
  const auto& f = std::get<float>(n.vals);

  printf("float is %f\n", f);
}

void print_node(const json::node& n) {

  switch (n.type) {
  case json::VALUE::TRUE:
    printf("true\n");
    break;
  case json::VALUE::FALSE:
    printf("false\n");
    break;
  case json::VALUE::ARRAY:
    print_array(n);
    break;
  case json::VALUE::OBJECT:
    print_object(n);
    break;
  case json::VALUE::NUMBER:
    print_number(n);
    break;
  case json::VALUE::NULL_TYPE:
    printf("null\n");
    break;
  case json::VALUE::STRING:
    print_string(n);
    break;
  }
}

void test_parse() {
  json::parser p(filename);

  if (!p.file_good()) {
    printf("file not good\n");
    return;
  }
  
  auto [nodeptr, status] = p.parse();

  if (status == json::STATUS::EMPTY) {
    printf("empty file\n");
    return;
  }
  if (status == json::STATUS::ERR) {
    printf("err\n");
    return;
  }
  
  print_node(*nodeptr);
}

int main() {
  // test_tokenizer();
  test_parse();
}
