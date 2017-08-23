// RUN: %clang_cc1 -std=c++1z -fcxx-exceptions -fexceptions -verify %s
// expected-no-diagnostics

#define assert(...) ((__VA_ARGS__) ? ((void)0) : throw 42)
#define CURRENT_FROM_MACRO() SL::current()
#define FORWARD(...) __VA_ARGS__

constexpr const int x = __builtin_LINE();
