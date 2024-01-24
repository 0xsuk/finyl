#include <string>
#include <optional>

struct noerror_t {};

constexpr noerror_t noerror;

template<typename T = int>
struct error {
  std::string message;
  std::optional<T> type;
  bool has = false;

  error(): has(false) {};
  error(noerror_t n): has(false) {};
  error(std::string _m): message(_m), has(true) {};
  error(T _type): type(_type), has(true) {};
  error(std::string _m, T _type): message(_m), type(_type), has(true) {};

  explicit operator bool() const {
    return has;
  }
};
