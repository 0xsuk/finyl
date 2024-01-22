#include "json.h"

namespace json {

tokenizer::tokenizer(std::string_view filename): file(filename.data()) {
}

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

  return { std::move(s), token_type::STRING };
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
  
  return { std::move(s), token_type::NUMBER };
}

void tokenizer::advance(int i) {
  auto pos = file.tellg();
  file.seekg((int)pos + i);
}

token tokenizer::get() {
  if (file.eof()) {
    return {"", token_type::END};
  }
  
  char c = skip();
  
  if (file.eof()) {
    return {"", token_type::END};
  }
  if (c == '"') {
    return string();
  }
  if (is_number(c)) {
    return number(c);
  }
  if (c == '{') {
    return {"", token_type::CURLY_OPEN};
  }
  if (c == '}') {
    return {"", token_type::CURLY_CLOSE};
  }
  if (c == ':') {
    return {"", token_type::COLON};
  }
  if (c == ',') {
    return {"", token_type::COMMA};
  }
  if (c == 't') {
    advance(4);
    return {"", token_type::TRUE};
  }
  if (c == 'f') {
    advance(5);
    return {"", token_type::FALSE};
  }
  if (c == 'n') {
    advance(4);
    return {"", token_type::NULL_TYPE};
  }
  if (c == '[') {
    return {"", token_type::ARRAY_OPEN};
  }
  if (c == ']') {
    return {"", token_type::ARRAY_CLOSE};
  }
  
  return token{"Unknown token", token_type::ERR};
}

}
