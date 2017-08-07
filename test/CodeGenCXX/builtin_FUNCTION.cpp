// RUN: %clang_cc1 -std=c++1z -fblocks %s -triple %itanium_abi_triple -emit-llvm -o - | FileCheck %s

namespace test_func {
constexpr const char *get_func(const char *f = __builtin_FUNCTION()) {
  return f;
}
constexpr const char *test_func_one() {
  return __builtin_FUNCTION();
}
constexpr const char *test_func_two() {
  return get_func();
}
const char *test_one = test_func_one();
const char *test_two = test_func_two();

struct Pair {
  const char *first;
  const char *second;
};
Pair test_same() {
  return Pair{__builtin_FUNCTION(), __func__};
}
const char *test_three = test_same().first;
const char *test_four = test_same().second;
} // namespace test_func
