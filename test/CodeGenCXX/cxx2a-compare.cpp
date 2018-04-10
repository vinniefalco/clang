// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | \
// RUN:    FileCheck %s \
// RUN:          '-DSTRONG_ORD="class.std::__1::strong_ordering"' \
// RUN:           -DSO_EQ=_ZNSt3__115strong_ordering5equalE \
// RUN:           -DSO_LESS=_ZNSt3__115strong_ordering4lessE \
// RUN:           -DSO_GREATER=_ZNSt3__115strong_ordering7greaterE \
// RUN:          '-DSTRONG_EQ="class.std::__1::strong_equality"' \
// RUN:           -DSE_EQ=_ZNSt3__115strong_equality5equalE \
// RUN:           -DSE_NE=_ZNSt3__115strong_equality8nonequalE \
// RUN:          '-DPO="class.std::__1::partial_ordering"' \
// RUN:           -DPO_EQ=_ZNSt3__116partial_ordering10equivalentE \
// RUN:           -DPO_LESS=_ZNSt3__116partial_ordering4lessE \
// RUN:           -DPO_GREATER=_ZNSt3__116partial_ordering7greaterE \
// RUN:           -DPO_UNORD=_ZNSt3__116partial_ordering9unorderedE


#include "Inputs/std-compare.h"

// CHECK-LABEL: @_Z11test_signedii
auto test_signed(int x, int y) {
  // CHECK: %retval = alloca %[[STRONG_ORD]]
  // CHECK: %cmp = icmp slt i32 %0, %1
  // CHECK: %[[SELECT1:.*]] = select i1 %cmp, %[[STRONG_ORD]]* @[[SO_LESS]], %[[STRONG_ORD]]* @[[SO_EQ]]
  // CHECK: %cmp1 = icmp sgt i32 %0, %1
  // CHECK: %[[SELECT2:.*]] = select i1 %cmp1, %[[STRONG_ORD]]* @[[SO_GREATER]], %[[STRONG_ORD]]* %[[SELECT1]]
  // CHECK: %[[TMPRET:.*]] = bitcast %[[STRONG_ORD]]* %retval
  // CHECK: %[[TMPSRC:.*]] = bitcast %[[STRONG_ORD]]* %[[SELECT2]]
  // CHECK: call void @llvm.memcpy{{.*}}(i8* align 1 %[[TMPRET]], i8* align 1 %[[TMPSRC]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13test_unsignedjj
auto test_unsigned(unsigned x, unsigned y) {
  // CHECK: %cmp = icmp ult i32 %0, %1
  // CHECK: %[[SELECT1:.*]] = select i1 %cmp, %[[STRONG_ORD]]* @[[SO_LESS]], %[[STRONG_ORD]]* @[[SO_EQ]]
  // CHECK: %cmp1 = icmp ugt i32 %0, %1
  // CHECK: %[[SELECT2:.*]] = select i1 %cmp1, %[[STRONG_ORD]]* @[[SO_GREATER]], %[[STRONG_ORD]]* %[[SELECT1]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z10float_testdd
auto float_test(double x, double y) {
  // CHECK: %retval = alloca %[[PO]]
  // CHECK: %[[CMP1:.*]] = fcmp ord double %0, %1
  // CHECK: %[[SELECT1:.*]] = select i1 %[[CMP1]], %[[PO]]* @[[PO_EQ]], %[[PO]]* @[[PO_UNORD]]
  // CHECK: %[[CMP2:.*]] = fcmp olt double %0, %1
  // CHECK: %[[SELECT2:.*]] = select i1 %[[CMP2]], %[[PO]]* @[[PO_LESS]], %[[PO]]* %[[SELECT1]]
  // CHECK: %[[CMP3:.*]] = fcmp ogt double %0, %1
  // CHECK: %[[SELECT3:.*]] = select i1 %[[CMP3]], %[[PO]]* @[[PO_GREATER]], %[[PO]]* %[[SELECT2]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z8ptr_testPiS_
auto ptr_test(int *x, int *y) {
  // CHECK: %cmp = icmp ult i32* %0, %1
  // CHECK: %[[SELECT1:.*]] = select i1 %cmp, %[[STRONG_ORD]]* @[[SO_LESS]], %[[STRONG_ORD]]* @[[SO_EQ]]
  // CHECK: %cmp1 = icmp ugt i32* %0, %1
  // CHECK: %[[SELECT2:.*]] = select i1 %cmp1, %[[STRONG_ORD]]* @[[SO_GREATER]], %[[STRONG_ORD]]* %[[SELECT1]]
  // CHECK: ret
  return x <=> y;
}

struct MemPtr {};
using MemPtrT = void (MemPtr::*)();
using MemDataT = int(MemPtr::*);

// CHECK-LABEL: @_Z12mem_ptr_testM6MemPtrFvvES1_
auto mem_ptr_test(MemPtrT x, MemPtrT y) {
  // CHECK: %retval = alloca %[[STRONG_EQ]]
  // CHECK: %cmp.ptr = icmp ne i64 %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp ne i64 %lhs.memptr.ptr, 0
  // CHECK: %cmp.adj = icmp ne i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = and i1 %cmp.ptr.null, %cmp.adj
  // CHECK: %memptr.ne = or i1 %cmp.ptr, %6
  // CHECK: select i1 %memptr.ne, %[[STRONG_EQ]]* @[[SE_NE]], %[[STRONG_EQ]]* @[[SE_EQ]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13mem_data_testM6MemPtriS0_
auto mem_data_test(MemDataT x, MemDataT y) {
  // CHECK: %retval = alloca %[[STRONG_EQ]]
  // CHECK: %[[CMP:.*]] = icmp ne i64 %0, %1
  // CHECK: select i1 %[[CMP]], %[[STRONG_EQ]]* @[[SE_NE]], %[[STRONG_EQ]]* @[[SE_EQ]]
  return x <=> y;
}
