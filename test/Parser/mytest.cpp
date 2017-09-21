// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1z %s
// expected-no-diagnostics

constexpr int bar() { return 0; }
constexpr void foo(int x)[[clang::diagnose_if(++x, "foo", "warning")]] {}
constexpr void foo2(int x)  { foo(x); }

static_assert((foo2(bar()), false), "");
