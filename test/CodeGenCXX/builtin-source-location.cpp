// RUN: %clang_cc1 -std=c++2a -fblocks %s -triple %itanium_abi_triple -emit-llvm -o %t.ll
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-BASIC-GLOBAL
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-LOCAL
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-INIT
// RUN: FileCheck --input-file %t.ll %s --check-prefix=CHECK-INIT-FUNC

#line 8 "builtin-source-location.cpp"

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
extern "C" int sink(...);

constexpr SL forward(SL sl = current()) {
  return sl;
}
SL forward_bad(SL sl = bad_current()) {
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
SL basic_global = current();

// CHECK-BASIC-GLOBAL-DAG: @basic_global_bad = global %struct.source_location zeroinitializer, align 8
// CHECK-BASIC-GLOBAL-DAG: @[[BAD_FILE:.*]] = private unnamed_addr constant {{.*}}c"test_basic_bad.cpp\00", align 1
// CHECK-BASIC-GLOBAL: @bad_current_marker
// CHECK-BASIC-GLOBAL: define internal void @__cxx_global_var_init
// CHECK-BASIC-GLOBAL-NEXT: entry:
// CHECK-BASIC-GLOBAL-NEXT: call void @_Z11bad_currentPKcS0_jj(%struct.source_location* sret @basic_global_bad,
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@[[BAD_FILE]],
// CHECK-BASIC-GLOBAL-SAME: {{.*}} i8* getelementptr inbounds ({{.*}}@.str.empty,
// CHECK-BASIC-GLOBAL-SAME: {{.*}}), i32 1101, i32 {{[0-9]+}}
// CHECK-BASIC-GLOBAL-NEXT: ret void
#line 1100 "test_basic_bad.cpp"
extern "C" void bad_current_marker() {}
SL basic_global_bad = bad_current();

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
  // CHECK-LOCAL: call void @_Z7currentPKcS0_jj(%struct.source_location* sret %local,
  // CHECK-LOCAL-SAME:{{.*}}@[[GOOD_FILE]]
  // CHECK-LOCAL-SAME:{{.*}}@[[FUNC_NAME]]
  // CHECK-LOCAL-SAME:{{.*}}, i32 2100, i32 {{[0-9]+}}
#line 2100 "test_current.cpp"
  SL local = current();
  // CHECK-LOCAL: call void @_Z11bad_currentPKcS0_jj(%struct.source_location* sret %bad_local,
  // CHECK-LOCAL-SAME:{{.*}}@[[BAD_FILE]]
  // CHECK-LOCAL-SAME:{{.*}}@[[FUNC_NAME]]
  // CHECK-LOCAL-SAME:{{.*}}, i32 2200, i32 {{[0-9]+}}
#line 2200 "test_bad_current.cpp"
  SL bad_local = bad_current();
#line 2300 "test_over.cpp"
  sink(local);
  sink(bad_local);
}

// CHECK-INIT-DAG: @GlobalInitDefault = global %struct.TestInit zeroinitializer, align 8
// CHECK-INIT-DAG: @GlobalInitVal = global %struct.TestInit zeroinitializer, align 8
// CHECK-INIT-DAG: @[[INIT_FILE:.*]] = private unnamed_addr constant [13 x i8] c"TestInit.cpp\00", align 1
#line 3000 "test_default_init.cpp"
extern "C" struct TestInit {
  SL info = current();
  SL bad_info = bad_current();
  SL nested_info = forward();
  SL arg_info;
  SL bad_arg_info;

#line 3100 "default.cpp"
  TestInit() = default;
#line 3200 "TestInit.cpp"
  TestInit(int, SL arg_info = current(),
           SL bad_arg_info = bad_current()) : arg_info(arg_info), bad_arg_info(bad_arg_info) {}
};
// CHECK-INIT: define void @global_init_marker()
extern "C" void global_init_marker() {}

// CHECK-INIT: define internal void @__cxx_global_var_init
// CHECK-INIT-NEXT: entry:
// CHECK-INIT-NEXT: call void @_ZN8TestInitC1Ev(%struct.TestInit* @GlobalInitDefault)
#line 3300 "GlobalInitDefault.cpp"
TestInit GlobalInitDefault;
int sink_global_init_default = sink(GlobalInitDefault);

// CHECK-INIT: define internal void @__cxx_global_var_init
//
// CHECK-INIT: call void @_Z7currentPKcS0_jj
// CHECK-INIT-SAME:{{.*}}@[[INIT_FILE]]
// CHECK-INIT-SAME:{{.*}}@.str.empty
// CHECK-INIT-SAME:{{.*}}, i32 3200, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_Z11bad_currentPKcS0_jj
// CHECK-INIT-SAME:{{.*}}@[[INIT_FILE]]
// CHECK-INIT-SAME:{{.*}}@.str.empty
// CHECK-INIT-SAME:{{.*}}, i32 3201, i32 {{[0-9]+}}
//
// CHECK-INIT: call void @_ZN8TestInitC1Ei15source_locationS0_(%struct.TestInit* @GlobalInitVal,
// CHECK-INIT-NEXT: ret void
#line 3400 "GlobalInitVal.cpp"
TestInit GlobalInitVal(42);
int sink_global_init_val = sink(GlobalInitVal);

extern "C" void test_init_function() {
#line 3500 "LocalInitDefault.cpp"
  TestInit InitDefault;
  TestInit InitVal(42);
  sink(InitDefault);
  sink(InitVal);
}

#line 4000 "test_init_constexpr.cpp"
extern "C" struct TestInitConstexpr {
  SL info = current();
  SL arg_info;
#line 4100 "TestInitConstexpr1.cpp"
  TestInitConstexpr() = default;
#line 4200 "TestInitConstexpr2.cpp"
  constexpr TestInitConstexpr(int, SL arg_info = current()) : arg_info(arg_info) {}
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
  SL info = current();
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
