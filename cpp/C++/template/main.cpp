#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
using remove_cvref =
    typename std::remove_cv<typename std::remove_reference<T>::type>;

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T>
using remove_cvref =
    typename std::remove_cv<typename std::remove_reference<T>::type>;

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, typename S> struct has_io {
  template <typename T_>
  static constexpr auto helper(T_ *)
      -> std::is_same<decltype((std::declval<T_>().io(std::declval<S &>()))),
                      void>;

  template <typename> static constexpr auto helper(...) -> std::false_type;

public:
  using type_ = decltype(helper<remove_cvref_t<T>>(nullptr));
  static constexpr bool value = type_::value;
};

template <typename T> struct IsOptional {
  inline static constexpr bool value = false;
};

template <typename T> struct IsOptional<std::optional<T>> {
  inline static constexpr bool value = true;
};

class BinarySerializer {
private:
  using Self = BinarySerializer;
  template <typename T>
  inline static constexpr bool is_elementary_type_v =
      !has_io<T, Self>::value && !std::is_pointer<T>::value &&
      !IsOptional<T>::value && !std::is_enum_v<T> && std::is_pod_v<T>;

  template <typename T, std::size_t n> using TArray = T[n];

public:
  std::vector<uint8_t> data;
  uint8_t *c_data;

  std::size_t head;
  std::size_t preserved;

  // std::string
  void operator()(const char *, const std::string &val) {
    std::vector<char> val_vector(val.begin(), val.end());
    this->operator()(nullptr, val_vector);
  }

  // C-array
  template <typename T, std::size_t n>
  void operator()(const char *, const TArray<T, n> &val) {
    for (std::size_t i = 0; i < n; i++) {
      this->operator()("", val[i]);
    }
  }

  // Elementary data types
  template <typename T>
  typename std::enable_if<is_elementary_type_v<T>, void>::type
  operator()(const char *, const T &val) {
    static_assert(!std::is_reference<T>::value, "T cannot be reference");
    static_assert(!std::is_const<T>::value, "T cannot be const");
    static_assert(!std::is_volatile<T>::value, "T cannot be volatile");
    static_assert(!std::is_pointer<T>::value, "T cannot be pointer");
    static_assert(std::is_pod_v<T>, "not pod");
    std::size_t new_size = head + sizeof(T);
    if (c_data) {
      std::memcpy(&c_data[head], &val, sizeof(T));
    } else {
      data.resize(new_size);
      std::memcpy(&data[head], &val, sizeof(T));
    }

    head += sizeof(T);
  }

  template <typename T>
  typename std::enable_if<has_io<T, Self>::value, void>::type
  operator()(const char *, const T &val) {
    val.io(*this);
  }

  // Unique Pointers to non-taichi-unit Types
  template <typename T>
  void operator()(const char *, const std::unique_ptr<T> &val) {
    this->operator()(ptr_to_int(val.get()));
    if (val.get() != nullptr) {
      this->operator()("", *val);
    }
  }

  template <typename T> std::size_t ptr_to_int(T *t) {
    return reinterpret_cast<std::size_t>(t);
  }

  // Raw pointers (no ownership)
  template <typename T>
  typename std::enable_if<std::is_pointer<T>::value, void>::type
  operator()(const char *, const T &val) {
    this->operator()("", ptr_to_int(val));
  }

  // enum class
  template <typename T>
  typename std::enable_if<std::is_enum_v<T>, void>::type
  operator()(const char *, const T &val) {
    using UT = std::underlying_type_t<T>;
    // https://stackoverflow.com/a/62688905/12003165
    this->operator()(nullptr, static_cast<UT>(val));
  }

  // std::vector
  template <typename T>
  void operator()(const char *, const std::vector<T> &val) {
    this->operator()("", val.size());
    for (std::size_t i = 0; i < val.size(); i++) {
      this->operator()("", val[i]);
    }
  }

  // std::pair
  template <typename T, typename G>
  void operator()(const char *, const std::pair<T, G> &val) {
    this->operator()(nullptr, val.first);
    this->operator()(nullptr, val.second);
  }

  // std::map
  template <typename K, typename V>
  void operator()(const char *, const std::map<K, V> &val) {
    handle_associative_container(val);
  }

  // std::unordered_map
  template <typename K, typename V>
  void operator()(const char *, const std::unordered_map<K, V> &val) {
    handle_associative_container(val);
  }

  // std::optional
  template <typename T>
  void operator()(const char *, const std::optional<T> &val) {
    this->operator()(nullptr, val.has_value());
    if (val.has_value()) {
      this->operator()(nullptr, val.value());
    }
  }

  template <typename T, typename... Args>
  void operator()(const char *, const T &t, Args &&... rest) {
    this->operator()(nullptr, t);
    this->operator()(nullptr, std::forward<Args>(rest)...);
  }

  template <typename T> void operator()(const T &val) {
    this->operator()(nullptr, val);
  }

private:
  template <typename M> void handle_associative_container(const M &val) {
    this->operator()(nullptr, val.size());
    for (auto iter : val) {
      auto first = iter.first;
      this->operator()(nullptr, first);
      this->operator()(nullptr, iter.second);
    }
  }
};

#define TI_IO(...)                                                             \
  { serializer(#__VA_ARGS__, __VA_ARGS__); }

#define TI_IO_DEF(...)                                                         \
  template <typename S> void io(S &serializer) const { TI_IO(__VA_ARGS__) }

struct Parent {
  struct Child {
    int a{0};
    float b{0.0f};
    bool c{false};

    bool operator==(const Child &other) const {
      return a == other.a && b == other.b && c == other.c;
    }

    // TI_IO_DEF(a, b, c);
  };

  std::optional<Child> b;
  std::string c;

  bool operator==(const Parent &other) const {
    return b == other.b && c == other.c;
  }

  // TI_IO_DEF(b, c);
};

int main() {
  Parent p;
  BinarySerializer bs;
  bs(p);
}