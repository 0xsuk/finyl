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

void print_node(std::unique_ptr<json::node>& n);
void print_object(json::object& o) {
  for (auto& [key, node] : o) {
    print_node(node);
    printf("and key is %s\n", key.data());
  }
}

void print_node(std::unique_ptr<json::node>& n) {
  auto& v = n->vals;

  if (std::holds_alternative<std::string>(v)) {
    printf("string %s\n", std::get<std::string>(v).data());
    return;
  }
  
  if (std::holds_alternative<json::object>(v)) {
    printf("node is object\n");
    json::object& o = std::get<json::object>(v);
    if (auto it = o.find("usb"); it != o.end()) {
      printf("found %s\n", it->first.data());
      std::unique_ptr<json::node> nn = std::move(it->second);
    }
    
  }
}

void test_parse() {
  json::parser p(filename);

  if (!p.file_good()) {
    printf("file not good\n");
    return;
  }
  
  auto [node, status] = p.parse();

  if (status == json::STATUS::EMPTY) {
    printf("empty file\n");
    return;
  }
  if (status == json::STATUS::ERR) {
    printf("err\n");
    return;
  }
  
  print_node(node);
}


int main() {
  // test_tokenizer();
  test_parse();
}
