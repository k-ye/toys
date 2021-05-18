#pragma once

#include <sstream>
#include <type_traits>

#include "serializer.h"

namespace taichi {

class TextOutputSerializer : public OutputSerializer<TextOutputSerializer> {
 public:
  TextOutputSerializer() : OutputSerializer<TextOutputSerializer>(this) {}

  void save_value(const std::string &s) { this->add_to_line(s); }

  template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  void save_value(const T &i) {
    this->add_to_line(std::to_string(i));
  }

  std::string get_result() const { return buffer_.str(); }

 private:
  void add_to_line(const std::string &s) { buffer_ << s << std::endl; }

  std::stringstream buffer_;
};

void save(TextOutputSerializer &ser, const std::string &s) {
  ser.save_value(s);
}

template <typename T,
          typename = std::enable_if_t<std::is_arithmetic_v<T>, void>>
inline void save(TextOutputSerializer &ser, const T &t) {
  ser.save_value(t);
}

}  // namespace taichi