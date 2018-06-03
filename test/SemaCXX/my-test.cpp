// Force x86-64 because some of our heuristics are actually based
// on integer sizes.

// RUN: %clang_cc1 -triple x86_64-apple-darwin -fcxx-exceptions -fsyntax-only -pedantic -verify -Wsign-compare -std=c++2a %s

#include "Inputs/std-compare.h"

namespace Test1 {
using FnTy = void();

struct T {
  int x;
};

auto operator<=>(T const &LHS, T const &RHS) {
  return LHS.x <=> RHS.x;
}

void test(T L, T R) {
  auto res = L < R;
}
} // namespace Test1
namespace Test2 {
enum class EnumA { A,
                   A2 };
enum class EnumB { B };

// expected-note@+1 {{candidate function}},
std::strong_ordering operator<=>(EnumA a, EnumB b) {
  return ((int)a <=> (int)b);
}
void test() {
  (void)(EnumB::B <=> EnumA::A); // OK, chooses reverse order synthesized candidate.
}
} // namespace Test2
