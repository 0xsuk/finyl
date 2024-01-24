#ifndef JSON_H
#define JSON_H

#include <vector>
#include <variant>
#include <map>
#include <string>
#include <memory>
#include <fstream>
#include <cmath>

namespace json {
  
enum class TOKEN {
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

enum class VALUE {
  OBJECT,
  ARRAY,
  STRING,
  NUMBER,
  TRUE,
  FALSE,
  NULL_TYPE,
};

enum class STATUS {
  EMPTY,
  ERR,
  OK,
};

struct token {
  std::string value = "";
  TOKEN type;
};

struct tokenizer {
  std::ifstream file;
  std::streampos position;

  tokenizer(std::string_view filename);
  token string();
  token number(char c);
  void advance(int);
  char skip();
  token get();
};

struct node;
using object = std::map<std::string, std::unique_ptr<node>>;
using array = std::vector<std::unique_ptr<node>>;
using values = std::variant<std::monostate, object, array, std::string, float>;

struct node {
  values vals;
  VALUE type;

  template<typename T>
  node(T&& v, VALUE t);
  node(VALUE t);
};

class parser {
  tokenizer t;
  
  std::unique_ptr<node> parse_string(const token& v);
  std::unique_ptr<node> parse_number(const token& v);
  std::unique_ptr<node> parse_object();
  std::unique_ptr<node> parse_array();
public:
  parser(std::string_view filename);
  bool file_good();
  std::pair<std::unique_ptr<node>, STATUS> parse();
};


bool is_string(const node* n);
const node* get_node(const node& n, const std::string& key);

template <typename T>
const T* get_if(const node& n, const std::string& key) {
  if (const node* found = get_node(n, key)) {
    return std::get_if<T>(&found->vals);
  }

  return nullptr;
}

}

#endif
