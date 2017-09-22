// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1z -fcontracts-ts %s

namespace test_parsing {
void foo(int x)[[expects:x == 0]];
void bar(int z)[[expects axiom:z != 1]];
//int baz(int x) [[expects axiom ret: ret != 1]];
} // namespace test_parsing

namespace test_parsing_bad {
void foo(int z)[[expects]]; // expected-error {{expected ':'}}
}

namespace test_constexpr_eval {
constexpr bool foo(int x)[[expects:x != 0]]
// expected-note@-1 {{from contract attribute on 'foo'}}
{ return true; }
static_assert(foo(0), "");
// expected-error@+1 {{FIXME contract failed}}
static_assert(foo(1), ""); // expected-error {{static_assert expression is not an integral constant expression}}

} // namespace test_constexpr_eval

namespace test_parsing_stmt {
bool foo(int x) {
  [[assert: x == 0]];
  return true;
}
}
