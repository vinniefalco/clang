// Force x86-64 because some of our heuristics are actually based
// on integer sizes.

// RUN: %clang_cc1 -triple x86_64-apple-darwin -fcxx-exceptions -fsyntax-only -pedantic -verify -Wsign-compare -std=c++2a %s

#include "Inputs/std-compare.h"

using FnTy = void();

struct T {
  FnTy *x;
};
auto operator<=>(T const &LHS, T const &RHS) {
  return LHS.x <=> RHS.x;
}

void test(T L, T R) {
  auto res = L < R;
}
