// RUN: %clang_cc1 -std=c++2a -verify %s -fcxx-exceptions -triple=x86_64-linux-gnu

#include "Inputs/std-compare.h"

namespace ThreeWayComparison {
  struct A {
    int n;
    constexpr friend int operator<=>(const A &a, const A &b) {
      return a.n < b.n ? -1 : a.n > b.n ? 1 : 0;
    }
  };
  static_assert(A{1} <=> A{2} < 0);
  static_assert(A{2} <=> A{1} > 0);
  static_assert(A{2} <=> A{2} == 0);

  static_assert(1 <=> 2 < 0);
  static_assert(2 <=> 1 > 0);
  static_assert(1 <=> 1 == 0);
  constexpr int k = (1 <=> 1, 0);
  // expected-warning@-1 {{three-way comparison result unused}}

  static_assert(std::strong_ordering::equal == 0);

  constexpr void f() {
    void(1 <=> 1);
  }

  struct MemPtr {
    void foo() {}
    void bar() {}
  };
  using MemPtrT = void (MemPtr::*)();

  using FnPtrT = void (*)();

  void FnPtr1() {}
  void FnPtr2() {}

#define CHECK(...) ((__VA_ARGS__) ? void() : throw "error")
#define CHECK_TYPE(...) static_assert(__is_same(__VA_ARGS__));

constexpr bool test_constexpr = [] {
  {
    auto &EQ = std::strong_ordering::equal;
    auto &LESS = std::strong_ordering::less;
    auto &GREATER = std::strong_ordering::greater;
    using SO = std::strong_ordering;
    auto eq = (42 <=> 42);
    CHECK_TYPE(decltype(eq), SO);
    CHECK(eq.test_eq(EQ));

    auto less = (-1 <=> 0);
    CHECK_TYPE(decltype(less), SO);
    CHECK(less.test_eq(LESS));

    auto greater = (42l <=> 1u);
    CHECK_TYPE(decltype(greater), SO);
    CHECK(greater.test_eq(GREATER));
  }
  {
    using PO = std::partial_ordering;
    auto EQUIV = PO::equivalent;
    auto LESS = PO::less;
    auto GREATER = PO::greater;

    auto eq = (42.0 <=> 42.0);
    CHECK_TYPE(decltype(eq), PO);
    CHECK(eq.test_eq(EQUIV));

    auto less = (39.0 <=> 42.0);
    CHECK_TYPE(decltype(less), PO);
    CHECK(less.test_eq(LESS));

    auto greater = (-10.123 <=> -101.1);
    CHECK_TYPE(decltype(greater), PO);
    CHECK(greater.test_eq(GREATER));
  }
  {
    using SE = std::strong_equality;
    auto EQ = SE::equal;
    auto NEQ = SE::nonequal;

    MemPtrT P1 = &MemPtr::foo;
    MemPtrT P12 = &MemPtr::foo;
    MemPtrT P2 = &MemPtr::bar;
    MemPtrT P3 = nullptr;

    auto eq = (P1 <=> P12);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(eq.test_eq(EQ));

    auto neq = (P1 <=> P2);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(neq.test_eq(NEQ));

    auto eq2 = (P3 <=> nullptr);
    CHECK_TYPE(decltype(eq2), SE);
    CHECK(eq2.test_eq(EQ));
  }
  {
    using SE = std::strong_equality;
    auto EQ = SE::equal;
    auto NEQ = SE::nonequal;

    FnPtrT F1 = &FnPtr1;
    FnPtrT F12 = &FnPtr1;
    FnPtrT F2 = &FnPtr2;
    FnPtrT F3 = nullptr;

    auto eq = (F1 <=> F12);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(eq.test_eq(EQ));

    auto neq = (F1 <=> F2);
    CHECK_TYPE(decltype(neq), SE);
    CHECK(neq.test_eq(NEQ));
  }
  {
    using SO = std::strong_ordering;
    auto EQ = SO::equal;
    auto LESS = SO::less;
    auto GREATER = SO::greater;

    int Arr[5] = {};
    using ArrRef = decltype((Arr));

    int *P1 = (Arr + 1);
    int *P12 = (Arr + 1);
    int *P2 = (Arr + 2);

    auto eq = (Arr <=> (ArrRef)Arr); //(P1 <=> P12);
    CHECK_TYPE(decltype(eq), SO);
    CHECK(eq.test_eq(EQ));
  }

  return true;
}();

// TODO: defaulted operator <=>
} // namespace ThreeWayComparison
