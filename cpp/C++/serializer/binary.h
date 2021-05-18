#pragma once

#include <cstring>
#include <type_traits>
#include <vector>
#include <memory>

#include "serializer.h"

namespace taichi {

class BinaryOutputSerializer : public OutputSerializer<BinaryOutputSerializer> {
 public:
  BinaryOutputSerializer() : OutputSerializer<BinaryOutputSerializer>(this) {
    constexpr int kSize = sizeof(head_);
    buffer_.resize(kSize);
    head_ = kSize;
  }

  void save_binary(const void* data, std::size_t size) {
    const auto nxt = head_ + size;
    if (nxt >= buffer_.size()) {
      buffer_.resize(nxt);
    }
    auto* dst = buffer_.data() + head_;
    head_ = nxt;
    std::memcpy(dst, data, size);
  }

 private:
  std::size_t head_{0};
  std::vector<char> buffer_;
};

// class BinaryInputSerializer : public InputSerializer<BinaryInputSerializer> {
//  public:
//   BinaryInputSerializer() : InputSerializer<BinaryInputSerializer>(this) {
//     constexpr int kSize = sizeof(head_);
//     buffer_.resize(kSize);
//     head_ = kSize;
//   }

//   void save_binary(const void* data, std::size_t size) {
//     const auto nxt = head_ + size;
//     if (nxt >= buffer_.size()) {
//       buffer_.resize(nxt);
//     }
//     auto* dst = buffer_.data() + head_;
//     head_ = nxt;
//     std::memcpy(dst, data, size);
//   }

//  private:
//   std::size_t head_{0};
//   std::vector<char> buffer_;
// };

template <class T>
inline typename std::enable_if<std::is_arithmetic_v<T>, void>::type save(
    BinaryOutputSerializer& ser, const T& t) {
  ser.save_binary(std::addressof(t), sizeof(t));
}

}  // namespace taichi
