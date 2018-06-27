// RUN: %clang_cc1 -std=c++2a -fblocks %s -triple %itanium_abi_triple -emit-llvm -o - | FileCheck %s
#line 3 "builtin-source-location.cpp"

// CHECK-DAG: @.str.file = private unnamed_addr constant [28 x i8] c"builtin-source-location.cpp\00", align 1
// CHECK-DAG: @.str.empty = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
// CHECK-DAG: @basic_global = global %struct.source_location { i32 1000, i32 30, i8* getelementptr inbounds ([28 x i8], [28 x i8]* @.str.file, i32 0, i32 0), i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.empty, i32 0, i32 0) }, align 8
// CHECK-DAG: @basic_global_bad = global %struct.source_location zeroinitializer, align 8
// CHECK-DAG: @.str.file.1 = private unnamed_addr constant [15 x i8] c"test_basic.cpp\00", align 1

extern "C" {
struct source_location {
private:
  unsigned int __m_line = 0;
  unsigned int __m_col = 0;
  const char *__m_file = nullptr;
  const char *__m_func = nullptr;

public:
  constexpr void set(unsigned l, unsigned c, const char *f, const char *func) {
    __m_line = l;
    __m_col = c;
    __m_file = f;
    __m_func = func;
  }
  constexpr source_location() = default;
  constexpr source_location(source_location const &) = default;
  constexpr unsigned int line() const noexcept { return __m_line; }
  constexpr unsigned int column() const noexcept { return __m_col; }
  constexpr const char *file() const noexcept { return __m_file; }
  constexpr const char *function() const noexcept { return __m_func; }
};
}
constexpr source_location current(
    const char *__file = __builtin_FILE(),
    const char *__func = __builtin_FUNCTION(),
    unsigned int __line = __builtin_LINE(),
    unsigned int __col = __builtin_COLUMN()) noexcept {
  source_location __loc;
  __loc.set(__line, __col, __file, __func);
  return __loc;
}
source_location bad_current(
    const char *__file = __builtin_FILE(),
    const char *__func = __builtin_FUNCTION(),
    unsigned int __line = __builtin_LINE(),
    unsigned int __col = __builtin_COLUMN()) noexcept {
  source_location __loc;
  __loc.set(__line, __col, __file, __func);
  return __loc;
}

using SL = source_location;

template <class T>
T launder(T val) { return val; }
extern "C" void sink(const source_location &);

#line 1000 "test_basic.cpp"
extern "C" SL basic_global = current();
extern "C" SL basic_global_bad = bad_current();
extern "C" void basic_use_globals() {
  sink(basic_global);
  sink(basic_global_bad);
}

#line 2000 "test_function.cpp"
extern "C" void test_function() {
#line 2100 "test_current.cpp"
  SL local = current();
#line 2200 "test_bad_current.cpp"
  SL bad_local = bad_current();
#line 2300 "test_over.cpp"
  sink(local);
  sink(bad_local);
}
