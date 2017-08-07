// RUN: %clang_cc1 -std=c++1z -fcxx-exceptions -fexceptions -verify %s

#define assert(...) ((__VA_ARGS__) ? ((void)0) : throw 42)

namespace std {
namespace experimental {
struct source_location {
private:
  unsigned int __m_line = 0;
  const char *__m_file = nullptr;
  const char *__m_func = nullptr;

public:
  static constexpr source_location current(
      const char *__file = __builtin_FILE(),
      const char *__func = __builtin_FUNCTION(),
      unsigned int __line = __builtin_LINE(),
      unsigned int __col = 0) noexcept {
    source_location __loc;
    __loc.__m_line = __line;
    __loc.__m_file = __file;
    __loc.__m_func = __func;
    return __loc;
  }
  constexpr source_location() = default;
  constexpr source_location(source_location const &) = default;
  constexpr unsigned int line() const noexcept { return __m_line; }
  constexpr unsigned int column() const noexcept { return 0; }
  constexpr const char *file() const noexcept { return __m_file; }
  constexpr const char *func() const noexcept { return __m_func; }
};
} // namespace experimental
} // namespace std

using SL = std::experimental::source_location;

constexpr bool is_equal(const char *LHS, const char *RHS) {
  while (*LHS != 0 && *RHS != 0) {
    if (*LHS != *RHS)
      return false;
    ++LHS;
    ++RHS;
  }
  return *LHS == 0 && *RHS == 0;
}

namespace test_line {

static_assert(SL::current().line() == __LINE__);

static constexpr SL GlobalS = SL::current();

static_assert(GlobalS.line() == __LINE__ - 2);

constexpr bool test_line_fn() {
  constexpr SL S = SL::current();
  static_assert(S.line() == (__LINE__ - 1), "");
  constexpr SL S2 = SL::current(

  );

  static_assert(S2.line() == (__LINE__ - 4), "");
  return true;
}
static_assert(test_line_fn());

static_assert(__builtin_LINE() == __LINE__, "");

constexpr int baz() { return 101; }

constexpr int test_line_fn_simple(int z = baz(), int x = __builtin_LINE()) {
  return x;
}
void bar() {
  static_assert(test_line_fn_simple() == __LINE__, "");
  static_assert(test_line_fn_simple() == __LINE__, "");
}

template <class T>
constexpr bool test_line_fn_template(T Expect, int L = __builtin_LINE()) {
  return Expect == L;
}
static_assert(test_line_fn_template(__LINE__));

struct InMemInit {
  int line = __builtin_LINE();
  int expect = 0;
  constexpr InMemInit() : expect(__LINE__) {}

  constexpr bool check() const {
    return line == expect;
  }
};

static_assert(InMemInit{}.check(), "");

} // namespace test_line

namespace test_file {
constexpr const char *test_file_simple(const char *__f = __builtin_FILE()) {
  return __f;
}
static_assert(is_equal(test_file_simple(), __FILE__));
static_assert(test_file_simple() != nullptr);
static_assert(!is_equal(test_file_simple(), "source_location.cpp"));
} // namespace test_file

namespace test_func {
constexpr const char *test_func_simple(const char *__f = __builtin_FUNCTION()) {
  return __f;
}
constexpr const char *get_func() {
  return __func__;
}
constexpr bool test_func() {
  return is_equal(__func__, test_func_simple()) &&
         !is_equal(get_func(), test_func_simple());
}
static_assert(test_func());
} // namespace test_func

// expected-no-diagnostics

