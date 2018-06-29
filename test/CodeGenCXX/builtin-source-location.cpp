// RUN: %clang_cc1 -std=c++2a -fblocks %s -triple %itanium_abi_triple -emit-llvm -o %t.ll

#line 8 "builtin-source-location.cpp"

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
  static constexpr source_location current(
      unsigned int __line = __builtin_LINE(),
      unsigned int __col = __builtin_COLUMN(),
      const char *__file = __builtin_FILE(),
      const char *__func = __builtin_FUNCTION()) noexcept {
    source_location __loc;
    __loc.set(__line, __col, __file, __func);
    return __loc;
  }
  static source_location bad_current(
      unsigned int __line = __builtin_LINE(),
      unsigned int __col = __builtin_COLUMN(),
      const char *__file = __builtin_FILE(),
      const char *__func = __builtin_FUNCTION()) noexcept {
    source_location __loc;
    __loc.set(__line, __col, __file, __func);
    return __loc;
  }
  constexpr source_location() = default;
  constexpr source_location(source_location const &) = default;
  constexpr unsigned int line() const noexcept { return __m_line; }
  constexpr unsigned int column() const noexcept { return __m_col; }
  constexpr const char *file() const noexcept { return __m_file; }
  constexpr const char *function() const noexcept { return __m_func; }
};

using SL = source_location;

template <class T>
T launder(T val) { return val; }
extern "C" int sink(...);

constexpr SL forward(SL sl = SL::current()) {
  return sl;
}
SL forward_bad(SL sl = SL::bad_current()) {
  return sl;
}

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-GLOBAL-ONE
//
// CHECK-GLOBAL-ONE-DAG: @basic_global = global %struct.source_location { i32 1000, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE:[^,]*]], {{[^@]*}}@[[FUNC:[^,]*]],
// CHECK-GLOBAL-ONE-DAG: @[[FILE]] = {{.*}}c"test_basic.cpp\00"
// CHECK-GLOBAL-ONE-DAG: @[[FUNC]] = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
#line 1000 "test_basic.cpp"
SL basic_global = SL::current();

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-GLOBAL-TWO
//
// CHECK-GLOBAL-TWO-DAG: @basic_global_bad = global %struct.source_location zeroinitializer, align 8
//
// CHECK-GLOBAL-TWO-DAG: call void @_ZN15source_location11bad_currentEjjPKcS1_(%struct.source_location* sret @basic_global_bad, i32 1100, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE:[^,]*]], {{[^@]*}}@[[FUNC:[^,]*]],
// CHECK-GLOBAL-TWO-DAG: @[[FILE]] = {{.*}}c"test_basic_bad.cpp\00"
// CHECK-GLOBAL-TWO-DAG: @[[FUNC]] = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
#line 1100 "test_basic_bad.cpp"
SL basic_global_bad = SL::bad_current();

#line 2000 "test_function.cpp"
extern "C" void test_function() {
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-LOCAL-ONE
//
// CHECK-LOCAL-ONE-DAG:  call void @_ZN15source_location7currentEjjPKcS1_(%struct.source_location* sret %local, i32 2100, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE:[^,]*]], {{[^@]*}}@[[FUNC:[^,]*]],
// CHECK-LOCAL-ONE-DAG: @[[FILE]] = {{.*}}c"test_current.cpp\00"
// CHECK-LOCAL-ONE-DAG: @[[FUNC]] = {{.*}}c"test_function\00"
#line 2100 "test_current.cpp"
  SL local = SL::current();

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-LOCAL-TWO
//
// CHECK-LOCAL-TWO-DAG: call void @_ZN15source_location11bad_currentEjjPKcS1_(%struct.source_location* sret %bad_local, i32 2200, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE:[^,]*]], {{[^@]*}}@[[FUNC:[^,]*]],
// CHECK-LOCAL-TWO-DAG: @[[FILE]] = {{.*}}c"test_bad_current.cpp\00"
// CHECK-LOCAL-TWO-DAG: @[[FUNC]] = {{.*}}c"test_function\00"
#line 2200 "test_bad_current.cpp"
  SL bad_local = SL::bad_current();
}

#line 3000 "TestInitClass.cpp"
struct TestInit {
  SL info = SL::current();
  SL nested_info = forward();
  SL arg_info;
  SL nested_arg_info;

#line 3100 "TestInitCtor.cpp"
  TestInit(SL arg_info = SL::current(),
           SL nested_arg_info = forward()) : arg_info(arg_info), nested_arg_info(nested_arg_info) {}
};

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-CTOR-GLOBAL "-DFUNC=.str.empty"
//
// CHECK-CTOR-GLOBAL-DAG: @GlobalInitVal = global %struct.TestInit zeroinitializer, align 8
// CHECK-CTOR-GLOBAL-DAG: @[[FILE:.*]] = {{.*}}c"GlobalInitVal.cpp\00"
//
// CHECK-CTOR-GLOBAL: define internal void @__cxx_global_var_init.4()
// CHECK-CTOR-GLOBAL-NOT: ret
//
// CHECK-CTOR-GLOBAL: call void @_ZN15source_location7currentEjjPKcS1_(%struct.source_location* sret %[[TMP_ONE:[^,]*]], i32 3400, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE]], {{[^@]*}}@[[FUNC]],
// CHECK-CTOR-GLOBAL-NEXT: call void @_ZN15source_location7currentEjjPKcS1_(%struct.source_location* sret %[[TMP_TWO:[^,]*]], i32 3400, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE]], {{[^@]*}}@[[FUNC]],
// CHECK-CTOR-GLOBAL-NEXT: call void @_Z7forward15source_location(%struct.source_location* sret %[[TMP_THREE:[^,]*]], %struct.source_location* byval align 8 %[[TMP_TWO]])
// CHECK-CTOR-GLOBAL-NEXT: call void @_ZN8TestInitC1E15source_locationS0_(%struct.TestInit* @GlobalInitVal, %struct.source_location* {{[^%]*}}%[[TMP_ONE]], %struct.source_location* {{[^%]*}}%[[TMP_THREE]]
#line 3400 "GlobalInitVal.cpp"
TestInit GlobalInitVal;

extern "C" void test_init_function() {
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-CTOR-LOCAL
//
// CHECK-CTOR-LOCAL-DAG: @[[FILE:.*]] = {{.*}}c"LocalInitVal.cpp\00"
// CHECK-CTOR-LOCAL-DAG: @[[FUNC:.*]] = {{.*}}c"test_init_function\00"
//
// CHECK-CTOR-LOCAL: define void @test_init_function()
// CHECK-CTOR-LOCAL-NOT: ret
//
// CHECK-CTOR-LOCAL: call void @_ZN15source_location7currentEjjPKcS1_(%struct.source_location* sret %[[TMP_ONE:[^,]*]], i32 3500, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE]], {{[^@]*}}@[[FUNC]],
// CHECK-CTOR-LOCAL-NEXT: call void @_ZN15source_location7currentEjjPKcS1_(%struct.source_location* sret %[[TMP_TWO:[^,]*]], i32 3500, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE]], {{[^@]*}}@[[FUNC]],
// CHECK-CTOR-LOCAL-NEXT: call void @_Z7forward15source_location(%struct.source_location* sret %[[TMP_THREE:[^,]*]], %struct.source_location* byval align 8 %[[TMP_TWO]])
// CHECK-CTOR-LOCAL-NEXT: call void @_ZN8TestInitC1E15source_locationS0_(%struct.TestInit* %init_local, %struct.source_location* {{[^%]*}}%[[TMP_ONE]], %struct.source_location* {{[^%]*}}%[[TMP_THREE]]
#line 3500 "LocalInitVal.cpp"
  TestInit init_local;
  sink(init_local);
}

#line 4000 "ConstexprClass.cpp"
extern "C" struct TestInitConstexpr {
  SL info = SL::current();
  SL arg_info;
#line 4100 "ConstexprCtorOne.cpp"
  TestInitConstexpr() = default;
#line 4200 "ConstexprCtorTwo.cpp"
  constexpr TestInitConstexpr(int, SL arg_info = SL::current()) : arg_info(arg_info) {}
};

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-CONSTEXPR-T1
//
// CHECK-CONSTEXPR-T1-DAG: @ConstexprGlobalDefault = global %struct.TestInitConstexpr { %struct.source_location { i32 4100, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE_NAME:[^,]*]], {{[^@]*}}@[[FUNC_NAME:[^,]*]]
// CHECK-CONSTEXPR-T1-DAG: @[[FILE_NAME]] = {{.*}}c"ConstexprCtorOne.cpp\00"
// CHECK-CONSTEXPR-T1-DAG: @[[FUNC_NAME]] = {{.*}}c"TestInitConstexpr\00"
#line 4300 "ConstexprGlobalDefault.cpp"
TestInitConstexpr ConstexprGlobalDefault;

// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-CONSTEXPR-T2
//
// CHECK-CONSTEXPR-T2-DAG: @ConstexprGlobalVal = global %struct.TestInitConstexpr { %struct.source_location { i32 4200, i32 {{[0-9]+}}, {{[^@]*}}@[[FILE_INIT:[^,]*]], {{[^@]*}}@[[FUNC_INIT:[^,]*]], {{[^%]*}}%struct.source_location { i32 4400, i32 {{[0-9]+}},  {{[^@]*}}@[[FILE_ARG:[^,]*]], {{[^@]*}}@.str.empty
// CHECK-CONSTEXPR-T2-DAG: @[[FILE_INIT]] = {{.*}}c"ConstexprCtorTwo.cpp\00"
// CHECK-CONSTEXPR-T2-DAG: @[[FUNC_INIT]] = {{.*}}c"TestInitConstexpr\00"
// CHECK-CONSTEXPR-T2-DAG: @[[FILE_ARG]] = {{.*}}c"ConstexprGlobalVal.cpp\00"
#line 4400 "ConstexprGlobalVal.cpp"
TestInitConstexpr ConstexprGlobalVal(42);

extern "C" void test_init_function_constexpr() {
#line 4500 "ConstexprFuncDefault.cpp"
  TestInitConstexpr InitDefaultConstexpr;

#line 4600 "ConstexprFuncVal.cpp"
  TestInitConstexpr InitValConstexpr(42);

  sink(ConstexprGlobalDefault);
  sink(ConstexprGlobalVal);
  sink(InitDefaultConstexpr);
  sink(InitValConstexpr);
}

#line 5000 "test_init_agg.cpp"
extern "C" struct TestInitAgg {
  SL info = SL::current();
#line 5100 "TestInitAgg.cpp"
};
#line 5200 "TestInitAggGlobal.cpp"
TestInitAgg GlobalInitDefaultAgg;
TestInitAgg GlobalInitValAgg = {};
extern "C" void test_init_function_agg() {
#line 5300 "TestInitAggFunc.cpp"
  TestInitAgg InitDefaultAgg;
  TestInitAgg InitValAgg = {};
  sink(GlobalInitDefaultAgg);
  sink(GlobalInitValAgg);
  sink(InitDefaultAgg);
  sink(InitValAgg);
}
