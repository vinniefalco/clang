// RUN: %clang_cc1 -std=c++1z -fcxx-exceptions -fexceptions -verify %s
// expected-no-diagnostics

#define assert(...) ((__VA_ARGS__) ? ((void)0) : throw 42)
#define CURRENT_FROM_MACRO() SL::current()
#define FORWARD(...) __VA_ARGS__

template <unsigned>
struct Printer;

namespace std {
namespace experimental {
struct source_location {
public:
  unsigned int __m_line = 0;

public:
  static source_location current(
      unsigned int __line = __builtin_LINE()) noexcept {
    source_location __loc;
    __loc.__m_line = __line;

    return __loc;
  }
  constexpr source_location() = default;
  constexpr source_location(source_location const &) = default;
  constexpr unsigned int line() const noexcept { return __m_line; }
};
} // namespace experimental
} // namespace std

using SL = std::experimental::source_location;

SL forwards(SL Val = SL::current()) {
  return Val;
}

extern "C" int sink(...);
namespace My_test {
#if 0
#line 3000 "test_default_init.cpp"
extern "C" struct TestInit {
  //SL info = SL::current();
  SL arg_info;
  SL bad_arg_info;

#line 3100 "default.cpp"
  TestInit() = default;
#line 3200 "TestInit.cpp"
//constexpr TestInit(int,
 //       SL arg_info = SL::current(),
 //       SL bad_arg_info = SL::current()) :
 //          arg_info(arg_info), bad_arg_info(bad_arg_info) {}
};

#line 3300 "GlobalInitDefault.cpp"
//TestInit GlobalInitDefault;
//int dummy = sink(GlobalInitDefault);

#line 3400 "GlobalInitVal.cpp"
#endif
void test() {
  volatile int x = forwards().__m_line;

  //TestInit GlobalInitVal(42);
  //sink(GlobalInitVal);
}

} // namespace My_test
