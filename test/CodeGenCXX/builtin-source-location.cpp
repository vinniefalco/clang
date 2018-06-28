// RUN: %clang_cc1 -std=c++2a -fblocks %s -triple %itanium_abi_triple -emit-llvm -o %t.ll
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-BASIC-GLOBAL
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-LOCAL
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-INIT
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-CONSTEXPR

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
      const char *__file = __builtin_FILE(),
      const char *__func = __builtin_FUNCTION(),
      unsigned int __line = __builtin_LINE(),
      unsigned int __col = __builtin_COLUMN()) noexcept {
    source_location __loc;
    __loc.set(__line, __col, __file, __func);
    return __loc;
  }
  static source_location bad_current(
      const char *__file = __builtin_FILE(),
      const char *__func = __builtin_FUNCTION(),
      unsigned int __line = __builtin_LINE(),
      unsigned int __col = __builtin_COLUMN()) noexcept {
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

// CHECK-BASIC-GLOBAL: @[[CUR_FILE:.+]] = private unnamed_addr constant [28 x i8] c"builtin-source-location.cpp\00", align 1
//
// CHECK-BASIC-GLOBAL: @basic_global = global %struct.source_location {
// CHECK-BASIC-GLOBAL-SAME: i32 1000, i32 {{[0-9]+}},
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@[[CUR_FILE]],
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@.str.empty,
//
#line 1000 "test_basic.cpp"
SL basic_global = SL::current();

// CHECK-BASIC-GLOBAL-DAG: @basic_global_bad = global %struct.source_location zeroinitializer, align 8
// CHECK-BASIC-GLOBAL-DAG: @[[BAD_FILE:.*]] = private unnamed_addr constant {{.*}}c"test_basic_bad.cpp\00", align 1
// CHECK-BASIC-GLOBAL: @bad_current_marker
// CHECK-BASIC-GLOBAL: define internal void @__cxx_global_var_init
// CHECK-BASIC-GLOBAL-NEXT: entry:
// CHECK-BASIC-GLOBAL-NEXT: call void @_ZN15source_location11bad_currentEPKcS1_jj(%struct.source_location* sret @basic_global_bad,
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@[[BAD_FILE]],
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@.str.empty,
// CHECK-BASIC-GLOBAL-SAME: {{.*}}), i32 1101, i32 {{[0-9]+}}
// CHECK-BASIC-GLOBAL-NEXT: ret void
#line 1100 "test_basic_bad.cpp"
extern "C" void bad_current_marker() {}
SL basic_global_bad = SL::bad_current();

extern "C" void basic_use_globals() {
  sink(basic_global);
  sink(basic_global_bad);
}

// CHECK-LOCAL-DAG: @[[GOOD_FILE:.*]] = private unnamed_addr constant [17 x i8] c"test_current.cpp\00", align 1
// CHECK-LOCAL-DAG: @[[BAD_FILE:.*]] = private unnamed_addr constant [21 x i8] c"test_bad_current.cpp\00", align 1
// CHECK-LOCAL-DAG: @[[FUNC_NAME:.*]] = private unnamed_addr constant [14 x i8] c"test_function\00"
// CHECK-LOCAL: define void @test_function()
#line 2000 "test_function.cpp"
extern "C" void test_function() {
  // CHECK-LOCAL: call void @_ZN15source_location7currentEPKcS1_jj(%struct.source_location* sret %local,
  // CHECK-LOCAL-SAME:{{.*}}@[[GOOD_FILE]]
  // CHECK-LOCAL-SAME:{{.*}}@[[FUNC_NAME]]
  // CHECK-LOCAL-SAME:{{.*}}, i32 2100, i32 {{[0-9]+}}
#line 2100 "test_current.cpp"
  SL local = SL::current();
  // CHECK-LOCAL: call void @_ZN15source_location11bad_currentEPKcS1_jj(%struct.source_location* sret %bad_local,
  // CHECK-LOCAL-SAME:{{.*}}@[[BAD_FILE]]
  // CHECK-LOCAL-SAME:{{.*}}@[[FUNC_NAME]]
  // CHECK-LOCAL-SAME:{{.*}}, i32 2200, i32 {{[0-9]+}}
#line 2200 "test_bad_current.cpp"
  SL bad_local = SL::bad_current();
#line 2300 "test_over.cpp"
  sink(local);
  sink(bad_local);
}

// CHECK-INIT-DAG: @GlobalInitDefault = global %struct.TestInit zeroinitializer, align 8
// CHECK-INIT-DAG: @GlobalInitVal = global %struct.TestInit zeroinitializer, align 8
// CHECK-INIT-DAG: @[[GLOBAL_VAL:.*]] = {{.*}}c"GlobalInitVal.cpp\00",
// CHECK-INIT-DAG: @[[INIT_FUNC:.*]] = {{.*}}c"test_init_function\00",
// CHECK-INIT-DAG: @[[INIT_CTOR_NAME:.*]] = {{.*}}c"TestInit\00",
// CHECK-INIT-DAG: @[[INIT_DEFAULT_CTOR:.*]] = {{.*}}"TestInitDefaultCtor.cpp\00",
// CHECK-INIT-DAG: @[[INIT_VAL_CTOR:.*]] = {{.*}}"TestInitValCtor.cpp\00",

#line 3000 "test_default_init.cpp"
struct TestInit {
  SL info = SL::current();
  SL bad_info = SL::bad_current();
  SL nested_info = forward();
  SL arg_info;
  SL bad_arg_info;

#line 3100 "TestInitDefaultCtor.cpp"
  TestInit() = default;
#line 3200 "TestInitValCtor.cpp"
  TestInit(int, SL arg_info = SL::current(),
           SL bad_arg_info = SL::bad_current()) : arg_info(arg_info), bad_arg_info(bad_arg_info) {}
};
// CHECK-INIT: define void @global_init_marker()
extern "C" void global_init_marker() {}

// CHECK-INIT: define internal void @__cxx_global_var_init
// CHECK-INIT-NEXT: entry:
// CHECK-INIT-NEXT: call void @_ZN8TestInitC1Ev(%struct.TestInit* @GlobalInitDefault)
#line 3300 "GlobalInitDefault.cpp"
TestInit GlobalInitDefault;

// CHECK-INIT: define internal void @__cxx_global_var_init
//
// CHECK-INIT: call void @_ZN15source_location7currentEPKcS1_jj
// CHECK-INIT-SAME:{{.*}}@[[GLOBAL_VAL]]
// CHECK-INIT-SAME:{{.*}}@.str.empty
// CHECK-INIT-SAME:{{.*}}, i32 3400, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_ZN15source_location11bad_currentEPKcS1_jj
// CHECK-INIT-SAME:{{.*}}@[[GLOBAL_VAL]]
// CHECK-INIT-SAME:{{.*}}@.str.empty
// CHECK-INIT-SAME:{{.*}}, i32 3400, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_ZN8TestInitC1Ei15source_locationS0_(%struct.TestInit* @GlobalInitVal,
// CHECK-INIT-NEXT: ret void
#line 3400 "GlobalInitVal.cpp"
TestInit GlobalInitVal(42);

extern "C" void test_init_function() {
#line 3500 "LocalInitDefault.cpp"
  TestInit InitDefault;
  TestInit InitVal(42);
  sink(InitDefault);
  sink(InitVal);
}

// CHECK-INIT: define linkonce_odr void @_ZN8TestInitC2Ev(
// CHECK-INIT: call void @_ZN15source_location7currentEPKcS1_jj(%struct.source_location* sret %info
// CHECK-INIT-SAME: {{.*}}@[[INIT_DEFAULT_CTOR]],
// CHECK-INIT-SAME: {{.*}}@[[INIT_CTOR_NAME]],
// CHECK-INIT-SAME: {{.*}}, i32 3100, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_ZN15source_location11bad_currentEPKcS1_jj(%struct.source_location* sret %bad_info
// CHECK-INIT-SAME: {{.*}}@[[INIT_DEFAULT_CTOR]],
// CHECK-INIT-SAME: {{.*}}@[[INIT_CTOR_NAME]],
// CHECK-INIT-SAME: {{.*}}, i32 3100, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_ZN15source_location7currentEPKcS1_jj(%struct.source_location* sret %[[TMP:[^,]*]],
// CHECK-INIT-SAME: {{.*}}@[[INIT_DEFAULT_CTOR]],
// CHECK-INIT-SAME: {{.*}}@[[INIT_CTOR_NAME]],
// CHECK-INIT-SAME: {{.*}}, i32 3100, i32 {{[0-9]+}}
//
// CHECK-INIT-NEXT: call void @_Z7forward15source_location({{.*}} %nested_info,{{.*}} %[[TMP]])

// CHECK-CONSTEXPR-DAG: @[[FILE_STR:.*]] = {{.*}} c"builtin-source-location.cpp\00"
// CHECK-CONSTEXPR-DAG: @GlobalInitDefaultConstexpr = global %struct.TestInitConstexpr { %struct.source_location { 32 4100, i32 3, {{.*}}@[[FILE_STR]], {{.*}}@.str.empty
// CHECK-CONSTEXPR-DAG: @GlobalInitValConstexpr = global %struct.TestInitConstexpr { %struct.source_location { i32 4200, i32 11, {{.*}}@[FILE_STR]], {{.*}}@.str.empty, cation { i32 4301, i32 19, i8* getelementptr inbounds ([28 x i8], [28 x i8]* @.str.file, i32 0, i32 0), i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.empty, i32 0, i32 0) } }, align 8
//@.str.file.9 = private unnamed_addr constant [26 x i8] c"TestInitConstexprFunc.cpp\00", align 1
//@.str.func.10 = private unnamed_addr constant [29 x i8] c"test_init_function_constexpr\00", align 1

#line 4000 "test_init_constexpr.cpp"
extern "C" struct TestInitConstexpr {
  SL info = SL::current();
  SL arg_info;
#line 4100 "TestInitConstexpr1.cpp"
  TestInitConstexpr() = default;
#line 4200 "TestInitConstexpr2.cpp"
  constexpr TestInitConstexpr(int, SL arg_info = SL::current()) : arg_info(arg_info) {}
};
#line 4300 "TestInitConstexprGlobal.cpp"
TestInitConstexpr GlobalInitDefaultConstexpr;
TestInitConstexpr GlobalInitValConstexpr(42);
extern "C" void test_init_function_constexpr() {
#line 4400 "TestInitConstexprFunc.cpp"
  TestInitConstexpr InitDefaultConstexpr;
  TestInit InitValConstexpr(42);
  sink(GlobalInitDefaultConstexpr);
  sink(GlobalInitValConstexpr);
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
