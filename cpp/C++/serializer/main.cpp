#include <iostream>
#include <string>
#include <vector>

// #include "binary.h"
#include "stl.h"
#include "text.h"
// #include "traits.h"

class Foo {
 public:
  template <typename S>
  void io(S &ser) {
    ser(a_, b_, c_);
  }

 private:
  int a_{0};
  float b_{42.0f};
  std::string c_{"abc"};
};

// template <typename S>
// void save(S &ser, const Foo &f) {
//   // ser(a_, b_, c_);
// }

class FakeS {};

int main() {
  using namespace taichi;
  TextOutputSerializer ser;

  Foo f;
  using SER = taichi::TextOutputSerializer;
  // using SER = taichi::TextOutputSerializer;
  using Checked = int;
  ser(f);
  std::cout << ser.get_result() << std::endl;
  // f.io(ser);
  // std::cout << "HasMemberIo=" << traits::HasMemberIo<SER, Checked>::value
  //           << std::endl
  //           << "HasNonMemberIo="
  //           << traits::HasNonMemberIo<SER, Checked>::value << std::endl
  //           << "HasNonMemberSave="
  //           << traits::HasNonMemberSave<SER, Checked>::value << std::endl;

  return 0;
}
