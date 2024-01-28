#include "json.h"

namespace json {

template<typename T>
node::node(T&& v, VALUE t):
  vals(std::forward<T>(v)),
  type(t){};

node::node(VALUE t):
  vals(),
  type(t){};

tokenizer::tokenizer(std::string_view filename):
  file(filename.data()) {}

char tokenizer::skip() {
  char c;
  do {
    file.get(c);
  } while (c == ' ' || c == '\n');

  return c;
}

token tokenizer::string() {
  std::string s;
  char c;
  while (true) {
    file.get(c);

    if (c == '\\') {
      s+=c;
      file.get(c);
      s+=c;
      continue;
    }

    if (c == '"') break;

    s+=c;
  }

  return { std::move(s), TOKEN::STRING };
}

bool is_number(char c) {
  return '0' <= c && c <= '9';
}

token tokenizer::number(char c) {
  std::string s;

  do {
    s += c;
    file.get(c);
  } while (is_number(c));
  
  file.seekg((int)file.tellg() - 1);
  
  return { std::move(s), TOKEN::NUMBER };
}

void tokenizer::advance(int i) {
  auto pos = file.tellg();
  file.seekg((int)pos + i);
}

token tokenizer::get() {
  if (file.eof()) {
    return {"", TOKEN::END};
  }
  
  char c = skip();
  
  if (file.eof()) {
    return {"", TOKEN::END};
  }
  if (c == '"') {
    return string();
  }
  if (is_number(c)) {
    return number(c);
  }
  if (c == '{') {
    return {"", TOKEN::CURLY_OPEN};
  }
  if (c == '}') {
    return {"", TOKEN::CURLY_CLOSE};
  }
  if (c == ':') {
    return {"", TOKEN::COLON};
  }
  if (c == ',') {
    return {"", TOKEN::COMMA};
  }
  if (c == 't') {
    advance(4);
    return {"", TOKEN::TRUE};
  }
  if (c == 'f') {
    advance(5);
    return {"", TOKEN::FALSE};
  }
  if (c == 'n') {
    advance(4);
    return {"", TOKEN::NULL_TYPE};
  }
  if (c == '[') {
    return {"", TOKEN::ARRAY_OPEN};
  }
  if (c == ']') {
    return {"", TOKEN::ARRAY_CLOSE};
  }
  
  return token{"Unknown token", TOKEN::ERR};
}

parser::parser(std::string_view filename): t(filename) {};

static float str_to_float(std::string_view s) {
  float f = 0;
  
  for (int i = s.size()-1; i>=0; i--) {
    int factor = pow(10, s.size() - i - 1); //starts from 0
    char c = s[i];
    switch (c) {
    case '0': {
      break;
    }
    case '1':
      f += 1*factor;
      break;
    case '2':
      f+= 2*factor;
      break;
    case '3':
      f+= 3*factor;
      break;
    case '4':
      f+= 4*factor;
      break;
    case '5':
      f+= 5*factor;
      break;
    case '6':
      f+= 6*factor;
      break;
    case '7':
      f+= 7*factor;
      break;
    case '8':
      f+= 8*factor;
      break;
    case '9':
      f+= 9*factor;
      break;
    }
  }

  return f;
}

std::unique_ptr<node> parser::parse_array() {
  std::vector<std::unique_ptr<node>> arr;

  while (true) {
    const token& v = t.get();
    if (v.type == TOKEN::COMMA) {
      continue;
    }
    if (v.type == TOKEN::ARRAY_CLOSE) {
      break;
    }
    
    if (v.type == TOKEN::CURLY_OPEN) {
      arr.push_back(parse_object());
      continue;
    }
    
    if (v.type == TOKEN::STRING) {
      arr.push_back(parse_string(v));
      continue;
    }
    if (v.type == TOKEN::NUMBER) {
      arr.push_back(parse_number(v));
      continue;
    }
    if (v.type == TOKEN::TRUE) {
      arr.push_back(std::make_unique<node>(VALUE::TRUE));
      continue;
    }
    if (v.type == TOKEN::FALSE) {
      arr.push_back(std::make_unique<node>(VALUE::FALSE));
      continue;
    }
    if (v.type == TOKEN::ARRAY_OPEN) {
      arr.push_back(parse_array());
      continue;
    }
    if (v.type == TOKEN::NULL_TYPE) {
      arr.push_back(std::make_unique<node>(VALUE::NULL_TYPE));
      continue;
    }
  }

  return std::make_unique<node>(std::move(arr), VALUE::ARRAY);
}

std::unique_ptr<node> parser::parse_string(const token& v) {
  return std::make_unique<node>(std::move(v.value), VALUE::STRING);
}

std::unique_ptr<node> parser::parse_number(const token& v) {
  return std::make_unique<node>(str_to_float(v.value), VALUE::NUMBER);
}

std::unique_ptr<node> parser::parse_object() {
  object o;
  
  while (true) {
    const token& k = t.get();
    if (k.type == TOKEN::COMMA) {
      continue;
    }
    if (k.type == TOKEN::ERR) {
      break;
    }
    if (k.type == TOKEN::END) {
      break;
    }
    if (k.type == TOKEN::CURLY_CLOSE) {
      break;
    }

    t.get(); // consume :

    const token& v = t.get();
    switch (v.type) {
    case TOKEN::CURLY_OPEN: {
      o[k.value] = parse_object();
      break;
    }
    case TOKEN::STRING: {
      o[k.value] = parse_string(v);
      break;
    }
    case TOKEN::NUMBER: {
      o[k.value] = parse_number(v);
      break;
    }
    case TOKEN::TRUE: {
      o[k.value] = std::make_unique<node>(VALUE::TRUE);
      break;
    }
    case TOKEN::FALSE: {
      o[k.value] = std::make_unique<node>(VALUE::FALSE);
      break;
    }
    case TOKEN::NULL_TYPE: {
      o[k.value] = std::make_unique<node>(VALUE::NULL_TYPE);
      break;
    }
    case TOKEN::ARRAY_OPEN: {
      o[k.value] = parse_array();
      break;
    }
    default:
      break;
    }
  }

  return std::make_unique<node>(std::move(o), VALUE::OBJECT);
}

bool parser::file_good() {
  return t.file.good();
}

std::pair< std::unique_ptr<node>, STATUS > parser::parse() {
  const auto& tok = t.get();

  if (tok.type == TOKEN::END) {
    return std::make_pair(nullptr, STATUS::EMPTY);
  }
  if (tok.type == TOKEN::ERR) {
    return std::make_pair(nullptr, STATUS::ERR);
  }

  return std::make_pair(parse_object(), STATUS::OK);
}

bool is_string(const node* n) {
  if (n == nullptr) return false;

  return std::holds_alternative<std::string>(n->vals);
}

const node* get_node(const node& n, const std::string& key) {
  const auto* o = std::get_if<json::object>(&n.vals);
  if (o == nullptr) {
    return nullptr;
  }

  auto it = o->find(key);
  if (it == o->end()) return nullptr;

  return it->second.get();
}

}
