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

  return { std::move(s), token_type::STRING};
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
    return {"", token_type::TRUE};
  }
  if (c == 'f') {
    return {"", token_type::FALSE};
  }
  if (c == 'n') {
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
