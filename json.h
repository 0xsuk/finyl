#ifndef JSON_H
#define JSON_H

#include <vector>
#include <variant>
#include <map>
#include <string>
#include <memory>
#include <fstream>

namespace json {
  
enum class token_type {
  CURLY_OPEN,
  CURLY_CLOSE,
  COLON,
  STRING,
  NUMBER,
  ARRAY_OPEN,
  ARRAY_CLOSE,
  COMMA,
  TRUE,
  FALSE,
  NULL_TYPE,
  END,
  ERR,
};

struct token {
  std::string value = "";
  token_type type;
};

struct tokenizer {
  std::ifstream file;
  std::streampos position;

  tokenizer(std::string_view filename);
  token string();
  void advance();
  char skip();
  token get();
};

struct node;
using object = std::map<std::string, std::shared_ptr<node>>;
using list = std::vector<std::shared_ptr<node>>;

struct node {
  std::variant<object, std::string> values;
};

}

#endif
