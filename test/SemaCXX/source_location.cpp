// RUN: %clang_cc1 -std=c++1z -fcxx-exceptions -fexceptions -verify %s
// expected-no-diagnostics

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

#include "Inputs/source-location-file.h"
namespace SLF = source_location_file;

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

//===----------------------------------------------------------------------===//
//                            __builtin_LINE()
//===----------------------------------------------------------------------===//

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
  constexpr bool check(int expect) const {
    return info.line() == expect;
  }
  SL info = SL::current();
  InMemInit() = default;
  constexpr InMemInit(int) {}
};
static_assert(InMemInit{}.check(__LINE__ - 3), "");
static_assert(InMemInit{42}.check(__LINE__ - 3), "");

template <class T, class U = SL>
struct InMemInitTemplate {
  constexpr bool check(int expect) const {
    return info.line() == expect;
  }
  U info = U::current();
  InMemInitTemplate() = default;
  constexpr InMemInitTemplate(T) {}
  constexpr InMemInitTemplate(T, T) : info(U::current()) {}
  template <class V = U> constexpr InMemInitTemplate(T, T, T, V info = U::current())
      : info(info) {}
};
void test_mem_init_template() {
  constexpr int line_offset = 8;
  static_assert(InMemInitTemplate<int>{}.check(__LINE__ - line_offset), "");
  static_assert(InMemInitTemplate<unsigned>{42}.check(__LINE__ - line_offset), "");
  static_assert(InMemInitTemplate<unsigned>{42, 42}.check(__LINE__ - line_offset), "");
  static_assert(InMemInitTemplate<unsigned>{42, 42, 42}.check(__LINE__), "");
}

struct AggInit {
  int x;
  int y = __builtin_LINE();
  constexpr bool check(int expect) const {
    return y == expect;
  }
};
constexpr AggInit AI{42};
static_assert(AI.check(__LINE__ - 1), "");

template <class T, class U = SL>
struct AggInitTemplate {
  constexpr bool check(int expect) const {
    return expect == info.line();
  }
  T x;
  U info = U::current();
};

template <class T, class U = SL>
constexpr U test_fn_template(T, U u = U::current()) {
  return u;
}
void fn_template_tests() {
  static_assert(test_fn_template(42).line() == __LINE__, "");
}

struct TestMethodTemplate {
  template <class T, class U = SL, class U2 = SL>
  constexpr U get(T, U u = U::current(), U2 u2 = identity(U2::current())) const {
    assert(u.line() == u2.line());
    return u;
  }
};
void method_template_tests() {
  static_assert(TestMethodTemplate{}.get(42).line() == __LINE__, "");
}

} // namespace test_line

//===----------------------------------------------------------------------===//
//                            __builtin_FILE()
//===----------------------------------------------------------------------===//

namespace test_file {
constexpr const char *test_file_simple(const char *__f = __builtin_FILE()) {
  return __f;
}
void test_function() {
  static_assert(is_equal(test_file_simple(), __FILE__));
  static_assert(is_equal(SLF::test_function().file(), __FILE__), "");
  static_assert(is_equal(SLF::test_function_template(42).file(), __FILE__), "");

  static_assert(is_equal(SLF::test_function_indirect().file(), SLF::global_info.file()), "");
  static_assert(is_equal(SLF::test_function_template_indirect(42).file(), SLF::global_info.file()), "");

  static_assert(test_file_simple() != nullptr);
  static_assert(!is_equal(test_file_simple(), "source_location.cpp"));
}
void test_class() {
  using SLF::TestClass;
  constexpr TestClass Default;
  constexpr TestClass InParam{42};
  constexpr TestClass Template{42, 42};
  static_assert(is_equal(Default.info.file(), SLF::FILE), "");
  static_assert(is_equal(InParam.info.file(), SLF::FILE), "");
  static_assert(is_equal(InParam.ctor_info.file(), __FILE__), "");
}

void test_aggr_class() {
  using Agg = SLF::AggrClass<>;
  constexpr Agg Default{};
  constexpr Agg InitOne{42};
  static_assert(is_equal(Default.init_info.file(), __FILE__), "");
  static_assert(is_equal(InitOne.init_info.file(), __FILE__), "");
}

} // namespace test_file

//===----------------------------------------------------------------------===//
//                            __builtin_FUNCTION()
//===----------------------------------------------------------------------===//

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

template <class T, class U = SL>
constexpr Pair<U, U> test_func_template(T, U u = U::current()) {
  static_assert(is_equal(__func__, U::current().func()));
  return {u, U::current()};
}
void func_template_tests() {
  constexpr auto P = test_func_template(42);
  static_assert(is_equal(P.first.func(), __func__), "");
  static_assert(!is_equal(P.second.func(), __func__), "");
}

template <class = int, class T = SL>
struct TestCtor {
  T info = T::current();
  T ctor_info;
  TestCtor() = default;
  template <class U = SL>
  constexpr TestCtor(int, U u = U::current()) : ctor_info(u) {}
};
void ctor_tests() {
  constexpr TestCtor<> Default;
  constexpr TestCtor<> Template{42};
  static_assert(!is_equal(Default.info.func(), __func__));
  static_assert(is_equal(Default.info.func(), "TestCtor"));
  static_assert(is_equal(Template.info.func(), "TestCtor"));
  static_assert(is_equal(Template.ctor_info.func(), __func__));
}

constexpr SL global_sl = SL::current();
static_assert(is_equal(global_sl.func(), ""));

} // namespace test_func
