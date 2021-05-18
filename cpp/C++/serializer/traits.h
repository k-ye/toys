#pragma once

namespace taichi {
namespace traits {
namespace detail {

enum class Sfinae {};

}

template <typename A, typename T>
struct HasMemberIo {
 private:
  template <typename S_, typename T_>
  static constexpr auto helper(T_ *)
      -> std::is_same<decltype((std::declval<T_>().io(std::declval<S_ &>()))),
                      void>;

  template <typename...>
  static constexpr auto helper(...) -> std::false_type;

  using decayed = std::decay_t<T>;

 public:
  using type = decltype(helper<A, decayed>(nullptr));
  static constexpr bool value = type::value;
};

template <typename A, typename T>
struct HasNonMemberIo {
 private:
  template <typename S_, typename T_>
  static constexpr auto helper(T_ *)
      -> std::is_same<decltype(io(std::declval<S_ &>(), std::declval<T_ &>())),
                      void>;

  template <typename...>
  static constexpr auto helper(...) -> std::false_type;

  using decayed = std::decay_t<T>;

 public:
  using type = decltype(helper<A, decayed>(nullptr));
  static constexpr bool value = type::value;
};

template <typename S, typename T>
struct HasNonMemberSave {
 private:
  template <typename S_, typename T_>
  static constexpr auto helper(T_ *) -> std::is_same<
      decltype(save(std::declval<S_ &>(), std::declval<const T_ &>())), void>;

  template <typename...>
  static constexpr auto helper(...) -> std::false_type;

  using decayed = std::decay_t<T>;

 public:
  using type = decltype(helper<S, decayed>(nullptr));
  static constexpr bool value = type::value;
};

template <typename S, typename T>
struct HasNonMemberLoad {
 private:
  template <typename S_, typename T_>
  static constexpr auto helper(T_ *) -> std::is_same<
      decltype(load(std::declval<S_ &>(), std::declval<T_ &>())), void>;

  template <typename...>
  static constexpr auto helper(...) -> std::false_type;

  using decayed = std::decay_t<T>;

 public:
  using type = decltype(helper<S, decayed>(nullptr));
  static constexpr bool value = type::value;
};

inline constexpr detail::Sfinae kSfinae = {};

template <bool B>
using EnableIf = typename std::enable_if<B, detail::Sfinae>::type;

}  // namespace traits
}  // namespace taichi
