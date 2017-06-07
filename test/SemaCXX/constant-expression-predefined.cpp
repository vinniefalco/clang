// RUN: %clang_cc1 -triple x86_64-linux -Wno-string-plus-int -Wno-pointer-arith -Wno-zero-length-array -fsyntax-only -fcxx-exceptions -verify -std=c++11 -pedantic %s -Wno-comment -Wno-tautological-pointer-compare -Wno-bool-conversion
#include <cassert>
#include <type_traits>

constexpr int foo(int) {
  static_assert(std::is_same<decltype(__constexpr__), bool>::value, "");
  if (__constexpr__)
    return 42;
  else
    return 101;
}

static_assert(foo(0) == 42, "");

extern int y;
int main() {
  assert(foo(y) == 101);
}
int y = 0;
