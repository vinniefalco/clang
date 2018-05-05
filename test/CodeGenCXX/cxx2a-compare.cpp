// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | \
// RUN:    FileCheck %s \
// RUN:          '-DSO="class.std::__1::strong_ordering"' \
// RUN:          '-DSE="class.std::__1::strong_equality"' \
// RUN:          '-DPO="class.std::__1::partial_ordering"' \
// RUN:           -DEQ=0 -DLT=-1 -DGT=1 -DUNORD=-127 -DNE=1 \

#include "Inputs/std-compare.h"

typedef int INT;

// CHECK-LABEL: @_Z11test_signedii
auto test_signed(int x, int y) {
  // CHECK: %retval = alloca %[[SO]]
  // CHECK: %cmp.lt = icmp slt i32 %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, i8 [[LT]], i8 [[GT]]
  // CHECK: %cmp.eq = icmp eq i32 %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, i8 [[EQ]], i8 %sel.lt
  // CHECK: %__value_ = getelementptr inbounds %[[SO]], %[[SO]]* %retval, i32 0, i32 0
  // CHECK: store i8 %sel.eq, i8* %__value_, align 1
  // CHECK: %[[FINAL:.*]] = getelemintptr inbounds %[[SO]], %[[SO]]* %retval

  // CHECK: call void @llvm.memcpy{{.*}}(i8* align 1 %[[TMPRET]], i8* align 1 %[[TMPSRC]]
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13test_unsignedjj
auto test_unsigned(unsigned x, unsigned y) {
  // CHECK: %cmp.lt = icmp ult i32 %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, i8 [[LT]], i8 [[GT]]
  // CHECK: %cmp.eq = icmp eq i32 %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, i8 [[EQ]], i8 %sel.lt
  // CHECK: %retval
  // CHECK: %sel.eq
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z10float_testdd
auto float_test(double x, double y) {
  // CHECK: %retval = alloca %[[PO]]
  // CHECK: %cmp.eq = fcmp oeq double %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, i8 [[EQ]], i8 [[UNORD]]
  // CHECK: %cmp.gt = fcmp ogt double %0, %1
  // CHECK: %sel.gt = select i1 %cmp.gt, i8 [[GT]], i8 %sel.eq
  // CHECK: %cmp.lt = fcmp olt double %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, i8 [[LT]], i8 %sel.gt
  // CHECK: %retval
  // CHECK: %sel.lt
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z8ptr_testPiS_
auto ptr_test(int *x, int *y) {
  // CHECK: %cmp.lt = icmp ult i32* %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, i8 [[LT]], i8 [[GT]]
  // CHECK: %cmp.eq = icmp eq i32* %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, i8 [[EQ]], i8 %sel.lt
  // CHECK: %retval
  // CHECK: %sel.eq
  // CHECK: ret
  return x <=> y;
}

struct MemPtr {};
using MemPtrT = void (MemPtr::*)();
using MemDataT = int(MemPtr::*);

// CHECK-LABEL: @_Z12mem_ptr_testM6MemPtrFvvES1_
auto mem_ptr_test(MemPtrT x, MemPtrT y) {
  // CHECK: %retval = alloca %[[SE]]
  // CHECK: %cmp.ptr = icmp eq i64 %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp eq i64 %lhs.memptr.ptr, 0
  // CHECK: %cmp.adj = icmp eq i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = or i1 %cmp.ptr.null, %cmp.adj
  // CHECK: %memptr.eq = and i1 %cmp.ptr, %6
  // CHECK: %sel.eq = select i1 %memptr.eq, i8 [[EQ]], i8 [[NE]]
  // CHECK: %retval
  // CHECK: %sel.eq
  // CHECK: ret
  return x <=> y;
}

// CHECK-LABEL: @_Z13mem_data_testM6MemPtriS0_
auto mem_data_test(MemDataT x, MemDataT y) {
  // CHECK: %retval = alloca %[[SE]]
  // CHECK: %[[CMP:.*]] = icmp eq i64 %0, %1
  // CHECK: %sel.eq = select i1 %[[CMP]], i8 [[EQ]], i8 [[NE]]
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

// CHECK-LABEL: @_Z16test_nullptr_objPiDn
auto test_nullptr_obj(int* x, decltype(nullptr) y) {
  // CHECK: %retval = alloca %[[SE]]
  // CHECK: %cmp.eq = icmp eq i32* %0, null
  // CHECK: %sel.eq = select i1 %cmp.eq, i8 [[EQ]], i8 [[NE]]
  // CHECK: %retval
  // CHECK: %sel.eq
  return x <=> y;
}

// CHECK-LABEL: @_Z18unscoped_enum_testijlm
void unscoped_enum_test(int i, unsigned u, long l, unsigned long ul) {
  enum EnumA : int { A };
  enum EnumB : unsigned { B };
  // CHECK: %[[I:.*]] = load {{.*}} %i.addr
  // CHECK: icmp slt i32 {{.*}} %[[I]]
  (void)(A <=> i);

  // CHECK: %[[U:.*]] = load {{.*}} %u.addr
  // CHECK: icmp ult i32 {{.*}} %[[U]]
  (void)(A <=> u);

  // CHECK: %[[L:.*]] = load {{.*}} %l.addr
  // CHECK: icmp slt i64 {{.*}} %[[L]]
  (void)(A <=> l);

  // CHECK: %[[U2:.*]] = load {{.*}} %u.addr
  // CHECK: icmp ult i32 {{.*}} %[[U2]]
  (void)(B <=> u);

  // CHECK: %[[UL:.*]] = load {{.*}} %ul.addr
  // CHECK: icmp ult i64 {{.*}} %[[UL]]
  (void)(B <=> ul);
}
