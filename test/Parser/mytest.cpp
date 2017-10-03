// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1z -fcontracts-ts %s

constexpr int bar() { return 0; }
constexpr void foo(int x)[[clang::diagnose_if(++x, "foo", "warning")]] {}
constexpr void foo2(int x)  { foo(x); }

//static_assert((foo2(bar()), false), "");

void baz(int x) {
  [[assert:x == 0]];
}

constexpr bool baz2(int x, int y, int z = 0)
    [[ensures:x == 2]] // expected-note {{from contract attribute on 'baz2'}}
    [[expects:x == 1]] // expected-note {{from contract attribute on 'baz2'}}
{
  x += y;
  [[assert:x != z]]; // expected-error {{Assertion 'assert : x != z' failed}}
  return true;
}
static_assert(baz2(1, 1));
// expected-error@+1 {{not an integral constant expression}}
static_assert(baz2(2, 1)); // expected-error {{Assertion 'expects : x == 1' failed}}
// expected-error@+1 {{not an integral constant expression}}
static_assert(baz2(1, 0)); // expected-error {{Assertion 'ensures : x == 2' failed}}
// expected-error@+1 {{not an integral constant expression}}
static_assert(baz2(1, 1, 2));
