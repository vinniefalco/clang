// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | FileCheck %s --check-prefix=ITANIUM

#include "Inputs/std-compare.h"

auto foo(int x, int y) {
  return x <=> y;
}

auto ptr_test(int *x, int *y) {
  return x <=> y;
}

struct MemPtr {
  void foo() {}
  void bar() {}
};
using MemPtrT = void (MemPtr::*)();

auto mem_ptr_test(MemPtrT x, MemPtrT y) {
  return x <=> y;
}
bool mem_ptr_test2(MemPtrT x, MemPtrT y) {
  return x != y;
}

auto float_test(double x, float y) {
  return x <=> y;
}
