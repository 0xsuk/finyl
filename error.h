#ifndef ERROR_H
#define ERROR_H


#include <string>
#include <optional>

struct noerror_t {};

constexpr noerror_t noerror;

enum class ERR {
  BADGE_NOT_VALID,
  JSON_FAILED,
  CANT_OPEN_COMMAND,
  CANT_CLOSE_COMMAND,
  COMMAND_FAILED,
  TOO_MANY_STEMS,
  FILE_DOES_NOT_EXIST,
  FAILED_TO_OPEN_FILE,
  FILE_TOO_LARGE,
  NO_SUCH_FILE,
  STEMS_DIFFERENT_SIZE,
  NO_SUCH_MIDI_DEVICE,
};

struct error {
  std::string message;
  std::optional<ERR> type;
  bool has = false;

  error(): has(false) {};
  error(noerror_t n): has(false) {};
  error(std::string _m): message(_m), has(true) {};
  error(ERR _type): type(_type), has(true) {};
  error(std::string _m, ERR _type): message(_m), type(_type), has(true) {};

  explicit operator bool() const {
    return has;
  }
};

#endif
