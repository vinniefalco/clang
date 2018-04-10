// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | \
// RUN:    FileCheck %s \
// RUN:          '-DSO="class.std::__1::strong_ordering"' \
// RUN:           -DSO_EQ=_ZNSt3__115strong_ordering5equalE \
// RUN:           -DSO_LT=_ZNSt3__115strong_ordering4lessE \
// RUN:           -DSO_GT=_ZNSt3__115strong_ordering7greaterE \
// RUN:          '-DSE="class.std::__1::strong_equality"' \
// RUN:           -DSE_EQ=_ZNSt3__115strong_equality5equalE \
// RUN:           -DSE_NE=_ZNSt3__115strong_equality8nonequalE \
// RUN:          '-DPO="class.std::__1::partial_ordering"' \
// RUN:           -DPO_EQ=_ZNSt3__116partial_ordering10equivalentE \
// RUN:           -DPO_LT=_ZNSt3__116partial_ordering4lessE \
// RUN:           -DPO_GT=_ZNSt3__116partial_ordering7greaterE \
// RUN:           -DPO_UNORD=_ZNSt3__116partial_ordering9unorderedE

#include "Inputs/std-compare.h"

typedef int INT;

// CHECK-LABEL: @_Z11test_signedii
auto test_signed(int x, int y) {
  // CHECK: %retval = alloca %[[SO]]
  // CHECK: %cmp.lt = icmp slt i32 %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, %[[SO]]* @[[SO_LT]], %[[SO]]* @[[SO_EQ]]
  // CHECK: %cmp.gt = icmp sgt i32 %0, %1
  // CHECK: %sel.gt = select i1 %cmp.gt, %[[SO]]* @[[SO_GT]], %[[SO]]* %sel.lt
  // CHECK: %[[TMPRET:.*]] = bitcast %[[SO]]* %retval
  // CHECK: %[[TMPSRC:.*]] = bitcast %[[SO]]* %sel.gt
  // CHECK: call void @llvm.memcpy{{.*}}(i8* align 1 %[[TMPRET]], i8* align 1 %[[TMPSRC]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13test_unsignedjj
auto test_unsigned(unsigned x, unsigned y) {
  // CHECK: %cmp.lt = icmp ult i32 %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, %[[SO]]* @[[SO_LT]], %[[SO]]* @[[SO_EQ]]
  // CHECK: %cmp.gt = icmp ugt i32 %0, %1
  // CHECK: %sel.gt = select i1 %cmp.gt, %[[SO]]* @[[SO_GT]], %[[SO]]* %sel.lt
  // CHECK: %retval
  // CHECK: %sel.gt
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z10float_testdd
auto float_test(double x, double y) {
  // CHECK: %retval = alloca %[[PO]]
  // CHECK: %cmp.le = fcmp ole double %0, %1
  // CHECK: %cmp.ge = fcmp oge double %0, %1
  // CHECK: %sel.lt = select i1 %cmp.le, %[[PO]]* @[[PO_LT]], %[[PO]]* @[[PO_GT]]
  // CHECK: %cmp.eq = and i1 %cmp.le, %cmp.ge
  // CHECK: %sel.eq = select i1 %cmp.eq, %[[PO]]* @[[PO_EQ]], %[[PO]]* @[[PO_UNORD]]
  // CHECK: %cmp.ord = xor i1 %cmp.le, %cmp.ge
  // CHECK: %sel.ord = select i1 %cmp.ord, %[[PO]]* %sel.lt, %[[PO]]* %sel.eq
  // CHECK: %retval
  // CHECK: %sel.ord
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z8ptr_testPiS_
auto ptr_test(int *x, int *y) {
  // CHECK: %cmp.lt = icmp ult i32* %0, %1
  // CHECK-NEXT: %sel.lt = select i1 %cmp.lt, %[[SO]]* @[[SO_LT]], %[[SO]]* @[[SO_EQ]]
  // CHECK: %cmp.gt = icmp ugt i32* %0, %1
  // CHECK-NEXT: %sel.gt = select i1 %cmp.gt, %[[SO]]* @[[SO_GT]], %[[SO]]* %sel.lt
  // CHECK: %retval
  // CHECK: %sel.gt
  // CHECK: ret
  return x <=> y;
}

struct MemPtr {};
using MemPtrT = void (MemPtr::*)();
using MemDataT = int(MemPtr::*);

// CHECK-LABEL: @_Z12mem_ptr_testM6MemPtrFvvES1_
auto mem_ptr_test(MemPtrT x, MemPtrT y) {
  // CHECK: %retval = alloca %[[SE]]
  // CHECK: %cmp.ptr = icmp ne i64 %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp ne i64 %lhs.memptr.ptr, 0
  // CHECK: %cmp.adj = icmp ne i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = and i1 %cmp.ptr.null, %cmp.adj
  // CHECK: %memptr.ne = or i1 %cmp.ptr, %6
  // CHECK: %sel.eq = select i1 %memptr.ne, %[[SE]]* @[[SE_NE]], %[[SE]]* @[[SE_EQ]]
  // CHECK: %retval
  // CHECK: %sel.eq
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13mem_data_testM6MemPtriS0_
auto mem_data_test(MemDataT x, MemDataT y) {
  // CHECK: %retval = alloca %[[SE]]
  // CHECK: %[[CMP:.*]] = icmp ne i64 %0, %1
  // CHECK: %sel.eq = select i1 %[[CMP]], %[[SE]]* @[[SE_NE]], %[[SE]]* @[[SE_EQ]]
  // CHECK: %retval
  // CHECK: %sel.eq
  return x <=> y;
}

// CHECK-LABEL: @_Z13test_constantv
auto test_constant() {
  // CHECK: entry:
  // CHECK-NOT: icmp
  // CHECK-NOT: [[SO_GT]]
  // CHECK-NOT: [[SO_EQ]]
  // CHECK: memcpy{{.*}}[[SO_LT]]
  // CHECK: ret
  const int x = 42;
  const int y = 101;
  return x <=> y;
}
