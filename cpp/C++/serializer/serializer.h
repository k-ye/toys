#pragma once

#include <type_traits>

#include "traits.h"

namespace taichi {
namespace detail {

class OutputSerializerBase {
 public:
  OutputSerializerBase() = default;
  virtual ~OutputSerializerBase() noexcept = default;
  OutputSerializerBase(OutputSerializerBase &&) noexcept = default;
  OutputSerializerBase &operator=(OutputSerializerBase &&) noexcept = default;
};

class InputSerializerBase {
 public:
  InputSerializerBase() = default;
  virtual ~InputSerializerBase() noexcept = default;
  InputSerializerBase(InputSerializerBase &&) noexcept = default;
  InputSerializerBase &operator=(InputSerializerBase &&) noexcept = default;
};

}  // namespace detail

template <class SerializerType>
class OutputSerializer : public detail::OutputSerializerBase {
 public:
  explicit OutputSerializer(SerializerType *derived) : self_(derived) {}

  template <class... Args>
  inline SerializerType &operator()(Args &&... args) {
    self_->process(std::forward<Args>(args)...);
    return *self_;
  }

 private:
  template <class T>
  inline void process(T &&head) {
    self_->process_impl(head);
  }

  template <class T, class... Args>
  inline void process(T &&head, Args &&... tail) {
    self_->process(std::forward<T>(head));
    self_->process(std::forward<Args>(tail)...);
  }

  template <typename T,
            traits::EnableIf<traits::HasMemberIo<SerializerType, T>::value> =
                traits::kSfinae>
  inline void process_impl(const T &val) {
    const_cast<T &>(val).io(*self_);
  }

  template <typename T,
            traits::EnableIf<traits::HasNonMemberIo<SerializerType, T>::value> =
                traits::kSfinae>
  inline void process_impl(const T &val) {
    io(*self_, const_cast<T &>(val));
  }

  template <typename T, traits::EnableIf<traits::HasNonMemberSave<
                            SerializerType, T>::value> = traits::kSfinae>
  inline void process_impl(const T &val) {
    save(*self_, val);
  }

  SerializerType *const self_;
};

}  // namespace taichi
