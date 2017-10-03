// RUN: %clang_cc1 -O0 -emit-llvm -fcontracts-ts -std=c++1z %s -o - | \
// RUN:   FileCheck %s

// CHECK-LABEL: define void @_Z3fooiRi(i32 %x, i32* dereferenceable(4) %y)
// CHECK:  %[[TMP:.*]] = load i32, i32* %x.addr, align 4
// CHECK-NEXT: %cmp = icmp eq i32 %[[TMP]], 0
// CHECK-NEXT: br i1 %cmp, label %if.then, label %if.end
// CHECK: if.then:
// CHECK-NEXT: call void @_ZSt9terminatev()
// CHECK: if.end:
// CHECK: if.end3:
// CHECK: store
// CHECK: %[[YTMP:.*]] = load i32*, i32** %y.addr, align 8
// CHECK-NEXT: %[[YTMP2:.*]] = load i32, i32* %[[YTMP]], align 4
// CHECK-NEXT: %[[CMP_ENSURES:.*]] = icmp ne i32 %[[YTMP2]], 0
// CHECK-NEXT: br i1 %[[CMP_ENSURES]], label %if.then
void foo(int x, int &y)
    [[expects:x == 0]]
    [[ensures:y != 0]] {
  int z = 42;
  [[assert:z == 42]];
  y = 101;
}
