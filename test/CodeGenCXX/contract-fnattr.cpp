// RUN: %clang_cc1 -O0 -emit-llvm -ftrapv -fcontracts-ts -std=c++1z %s -o - | \
// RUN:   FileCheck %s

// CHECK-LABEL: define void @_Z3fooi(i32 %x)
// CHECK:  %[[TMP:.*]] = load i32, i32* %x.addr, align 4
// CHECK-NEXT: %cmp = icmp eq i32 %[[TMP]], 0
// CHECK-NEXT: br i1 %cmp, label %if.then, label %if.end
// CHECK: if.then:
// CHECK-NEXT: call void @_ZSt9terminatev()
// CHECK: if.end:
// CHECK-NEXT: ret void
void foo(int x) [[ensures: x == 0]] {

}
