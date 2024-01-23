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
#include <optional>
//get pointer to existing node;
template<typename T>
const T* find_key(const json::node& n, const std::string& key) {
  auto& v = n.vals;
  if (std::holds_alternative<json::object>(v)) {
    const json::object& o = std::get<json::object>(v);
    if (auto it = o.find(key); it != o.end()) {
      return &std::get<T>(it->second->vals);
    }
  };

  return nullptr;
}

void print_object(const json::object& o) {
  for (auto& [key, node] : o) {
    print_node(*node);
    printf("and key is %s\n", key.data());
  }
}

void print_node(const json::node& n) {
  const auto& v = n.vals;

  if (std::holds_alternative<std::string>(v)) {
    printf("string %s\n", std::get<std::string>(v).data());
    return;
  }
  
  if (std::holds_alternative<json::object>(v)) {
    print_object(std::get<json::object>(v));
    return;
  }
  
  // const auto* track = find_key<json::object>(n, "track");
  // if (track == nullptr) {
  //   printf("track not found\n");
  //   return;
  // }
  // if (track != nullptr) {
  //   print_object(*track);
  // }
  
  // if (find_key<std::string>(n, "badge") != nullptr) {
    // printf("usb\n");
  // }
  
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
