// RUN: %clang_cc1 -std=c++1z -fcxx-exceptions -fexceptions -verify %s
// expected-no-diagnostics

#define assert(...) ((__VA_ARGS__) ? ((void)0) : throw 42)
#define CURRENT_FROM_MACRO() SL::current()
#define FORWARD(...) __VA_ARGS__

constexpr bool is_equal(const char *LHS, const char *RHS) {
  while (*LHS != 0 && *RHS != 0) {
    if (*LHS != *RHS)
      return false;
    ++LHS;
    ++RHS;
  }
  return *LHS == 0 && *RHS == 0;
}

template <class T>
constexpr T identity(T t) {
  return t;
}

template <class T, class U>
struct Pair {
  T first;
  U second;
};

template <class T, class U>
constexpr bool is_same = false;
template <class T>
constexpr bool is_same<T, T> = true;

// test types
static_assert(is_same<decltype(__builtin_LINE()), unsigned>);
static_assert(is_same<decltype(__builtin_COLUMN()), unsigned>);
static_assert(is_same<decltype(__builtin_FILE()), const char *>);
static_assert(is_same<decltype(__builtin_FUNCTION()), const char *>);

// test noexcept
static_assert(noexcept(__builtin_LINE()));
static_assert(noexcept(__builtin_COLUMN()));
static_assert(noexcept(__builtin_FILE()));
static_assert(noexcept(__builtin_FUNCTION()));

#line 500
constexpr const int x = __builtin_LINE();
const int x_static = __builtin_LINE();
static_assert(x == 500, "");

#line 600 "loc2.cpp"
constexpr const char* y = __builtin_FILE();
const char* const yy = __builtin_FILE();
const char* const yyy = __FILE__;
static_assert(is_equal(y, "loc2.cpp"), "");
static_assert(!is_equal(y, "abc"), "");

#line 10
const volatile void* volatile sink = nullptr;
void foo() {
  sink = &yy;
  sink = &yyy;
  sink = __FUNCTION__;
  sink = __FILE__;
  constexpr const char *_ = __FUNCTION__;
}

constexpr unsigned in_default(int x = __builtin_LINE()) {
  return x;
}
#line 700
static_assert(in_default() == 700);

constexpr unsigned in_body() {
#line 800
  return __builtin_LINE();
}
static_assert(in_body() == 800, "");
