// RUN: %clang_cc1 -pedantic-errors -std=c++2a -emit-pch  %s -o %t
// RUN: %clang_cc1 -pedantic-errors -std=c++2a -include-pch %t -verify %s

#ifndef HEADER
#define HEADER

#include "Inputs/std-compare.h"

#else

// expected-no-diagnostics
void foo() {
  (void)(42 <=> 101);
}

#endif
