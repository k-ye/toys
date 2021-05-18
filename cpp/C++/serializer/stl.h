#pragma once

// #include <optional>
#include <string>
#include <vector>

namespace taichi {
template <typename S, typename T>
void save(S& ser, const std::vector<T>& vec) {
  ser(vec.size());
  for (const auto& i : vec) {
    ser(i);
  }
}

template <typename S, typename T>
void save(S& ser, const std::string& str) {
  std::vector<char> vs(str.begin(), str.end());
  save(ser, vs);
}

}  // namespace taichi