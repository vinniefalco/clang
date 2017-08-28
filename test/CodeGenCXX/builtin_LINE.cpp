// RUN: %clang_cc1 -std=c++1z -fblocks %s -triple %itanium_abi_triple -emit-llvm -o - | FileCheck %s

extern "C" int sink;
extern "C" const volatile void* volatile ptr_sink = nullptr;

struct Tag1 {};
struct Tag2 {};
struct Tag3 {};
struct Tag4 {};

constexpr int get_line_constexpr(int l = __builtin_LINE()) {
  return l;
}

int get_line_nonconstexpr(int l = __builtin_LINE()) {
  return l;
}

// CHECK: @global_one = global i32 [[@LINE+1]], align 4
int global_one = __builtin_LINE();
// CHECK-NEXT: @global_two = global i32 [[@LINE+1]], align 4
int global_two = get_line_constexpr();
// CHECK: @_ZL12global_three = internal constant i32 [[@LINE+1]], align 4
const int global_three = get_line_constexpr();
// CHECK-LABEL: define internal void @__cxx_global_var_init
// CHECK: %call = call i32 @_Z21get_line_nonconstexpri(i32 [[@LINE+2]])
// CHECK-NEXT: store i32 %call, i32* @global_four, align 4
int global_four = get_line_nonconstexpr();

struct InClassInit {
  int Init = __builtin_LINE();
  InClassInit() = default;
  constexpr InClassInit(Tag1, int l = __builtin_LINE()) : Init(l) {}
  constexpr InClassInit(Tag2) : Init(__builtin_LINE()) {}
  InClassInit(Tag3, int l = __builtin_LINE()) : Init(l) {}
  InClassInit(Tag4) : Init(__builtin_LINE()) {}

  static void test_class();
};
// CHECK-LABEL: define void @_ZN11InClassInit10test_classEv()
void InClassInit::test_class() {
  // CHECK: call void @_ZN11InClassInitC1Ev(%struct.InClassInit* %test_one)
  InClassInit test_one;
  // CHECK-NEXT: call void @_ZN11InClassInitC1E4Tag1i(%struct.InClassInit* %test_two, i32 [[@LINE+1]])
  InClassInit test_two{Tag1{}};
  // CHECK-NEXT: call void @_ZN11InClassInitC1E4Tag2(%struct.InClassInit* %test_three)
  InClassInit test_three{Tag2{}};
  // CHECK-NEXT: call void @_ZN11InClassInitC1E4Tag3i(%struct.InClassInit* %test_four, i32 [[@LINE+1]])
  InClassInit test_four(Tag3{});
  // CHECK-NEXT: call void @_ZN11InClassInitC1E4Tag4(%struct.InClassInit* %test_five)
  InClassInit test_five{Tag4{}};
}

int get_line(int l = __builtin_LINE()) {
  return l;
}

// CHECK-LABEL: define void @_Z13get_line_testv()
void get_line_test() {
  // CHECK: %[[CALL:.+]] = call i32 @_Z8get_linei(i32 [[@LINE+2]])
  // CHECK-NEXT: store i32 %[[CALL]], i32* @sink, align 4
  sink = get_line();
  // CHECK-NEXT:  store i32 [[@LINE+1]], i32* @sink, align 4
  sink = __builtin_LINE();
  ptr_sink = &global_three;
}
