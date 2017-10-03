// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1z -fcontracts-ts %s
// expected-no-diagnostics

constexpr int bar() { return 0; }
constexpr void foo(int x)[[clang::diagnose_if(++x, "foo", "warning")]] {}
constexpr void foo2(int x)  { foo(x); }

//static_assert((foo2(bar()), false), "");

void baz(int x) {
  [[assert:x == 0]];
}

constexpr bool baz2(int x)
    [[ensures:x == 2]]
    [[expects:x == 1]] {
  //x = x + x;
  return true;
}
static_assert(baz2(1));
