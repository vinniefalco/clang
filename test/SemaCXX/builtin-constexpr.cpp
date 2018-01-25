// RUN: %clang_cc1 %s -fsyntax-only -verify -std=c++11 -fcxx-exceptions
// RUN: %clang_cc1 %s -fsyntax-only -verify -std=c++1z -fcxx-exceptions
constexpr bool isc() {
  return __builtin_constexpr();
}

static_assert(isc(), "");

constexpr bool pred(bool p) {
  if (p)
    return __builtin_constexpr();
  return false;
}
static_assert(pred(true), "");
