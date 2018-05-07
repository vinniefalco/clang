// RUN: %clang_cc1 -std=c++2a -emit-llvm %s -o - -triple %itanium_abi_triple | \
// RUN:    FileCheck %s \
// RUN:          '-DWE="class.std::__1::weak_equality"' \
// RUN:          '-DSO="class.std::__1::strong_ordering"' \
// RUN:          '-DSE="class.std::__1::strong_equality"' \
// RUN:          '-DPO="class.std::__1::partial_ordering"' \
// RUN:           -DEQ=0 -DLT=-1 -DGT=1 -DUNORD=-127 -DNE=1 \
// RUN:           -DOpaqueBaseSS=_ZNK10OpaqueBasessERKS_ \
// RUN:           -DOpaqueMemSS=_ZNK9OpaqueMemssERKS_ \
// RUN:           -DOpaqueFnSS=_ZssRK8OpaqueFnS1_ \
// RUN:           -DSO_NE=_ZNSt3__1neENS_15strong_orderingEMNS_19_CmpUnspecifiedTypeEFvvE \
// RUN:           -DSO_LT=_ZNSt3__1ltENS_15strong_orderingEMNS_19_CmpUnspecifiedTypeEFvvE

#include "Inputs/std-compare.h"

// CHECK-DAG: $_ZNK2T1ssERKS_ = comdat any
// CHECK-DAG: $_ZNK2T1ltERKS_ = comdat any

template <class T>
void use_cmp(T const &LHS, T const &RHS) {
  (void)(LHS <=> RHS);
  (void)(LHS < RHS);
}
struct OpaqueBase {
  std::strong_ordering operator<=>(OpaqueBase const &) const;
};
struct OpaqueMem {
  std::strong_ordering operator<=>(OpaqueMem const &) const;
};
struct OpaqueFn {
};
std::strong_ordering operator<=>(OpaqueFn const &, OpaqueFn const &);

struct T1 : OpaqueBase {
  OpaqueMem first;
  OpaqueFn second;
  // CHECK-LABEL: define linkonce_odr i8 @_ZNK2T1ssERKS_(
  auto operator<=>(T1 const &other) const = default;
  // CHECK: call i8 @[[OpaqueBaseSS]]
  // CHECK: %[[BASE_NE_ZERO:.*]] = call zeroext i1 @[[SO_NE]]
  // CHECK: br i1 %[[BASE_NE_ZERO]], label %if.then, label %if.end
  // CHECK: if.then:
  // CHECK-NEXT: br label %return
  // CHECK: if.end:
  // CHECK: call i8 @[[OpaqueMemSS]]
  // CHECK: %[[MEM_NE_ZERO:.*]] = call zeroext i1 @[[SO_NE]]
  // CHECK: br i1 %[[MEM_NE_ZERO]], label %[[THEN2:.*]], label %[[END2:.*]]
  // CHECK: [[THEN2]]:
  // CHECK-NEXT: br label %return
  // CHECK: [[END2]]:
  // CHECK: call i8 @[[OpaqueFnSS]]
  // CHECK-NOT: [[SO_NE]]
  // CHECK: br label %return
  // CHECK: return:
  // CHECK-NEXT: %[[TMP:.*]] = getelementptr inbounds %[[SO]], %[[SO]]* %retval
  // CHECK-NEXT: %[[TMP2:.*]] = load i8, i8* %[[TMP]]
  // CHECK-NEXT: ret i8 %[[TMP2]]

  // CHECK-LABEL: define linkonce_odr zeroext i1 @_ZNK2T1ltERKS_(
  bool operator<(T1 const &other) const = default;
  // CHECK: call i8 @_ZNK2T1ssERKS_(
  // CHECK: %[[RES:.*]] = call zeroext i1 @[[SO_LT]]
  // CHECK-NEXT: ret i1 %[[RES]]
};
template void use_cmp<T1>(T1 const &, T1 const &);
