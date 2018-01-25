// RUN: %clang_cc1 -triple=x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s
constexpr bool isc() {
  return __builtin_constexpr();
}

bool isc_nc() {
  return __builtin_constexpr();
}

// CHECK: blah
constexpr bool test1 = isc();
bool test2 = isc();
