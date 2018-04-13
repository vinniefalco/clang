// RUN: %clang_cc1 -pedantic-errors -std=c++2a -emit-pch  %s -o %t
// RUN: %clang_cc1 -pedantic-errors -std=c++2a -include-pch %t -verify %s

#ifndef HEADER
#define HEADER

#include "Inputs/std-compare.h"
constexpr auto foo() {
  return (42 <=> 101);
}

#else

// expected-no-diagnostics

static_assert(foo() < 0);

#endif
