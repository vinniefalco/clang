// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | FileCheck %s --check-prefix=ITANIUM

#include "Inputs/std-compare.h"

auto test_signed(int x, int y) {
  return x <=> y;
}

auto test_unsigned(unsigned x, unsigned y) {
  return x <=> y;
}

auto float_test(double x, float y) {
  return x <=> y;
}

auto ptr_test(int *x, int *y) {
  return x <=> y;
}

struct MemPtr {};
using MemPtrT = void (MemPtr::*)();
using MemDataT = int(MemPtr::*);

auto mem_ptr_test(MemPtrT x, MemPtrT y) {
  return x <=> y;
}

auto mem_data_test(MemDataT x, MemDataT y) {
  return x <=> y;
}
