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

namespace std {
inline namespace __1 {

struct NonTrivial {
  int Data;
  constexpr NonTrivial(int D = 0) : Data(D) {}
  constexpr NonTrivial(NonTrivial const &other) : Data(other.Data) {}
};

class partial_ordering {
  using _ValueT = signed char;
  explicit constexpr partial_ordering(signed char x) : __value_(x) {}

public:
  static const partial_ordering less;
  static const partial_ordering equivalent;
  static const partial_ordering greater;
  static const partial_ordering unordered;

  // Make this type non-trivially copyable
  partial_ordering(partial_ordering const &__other) = default;

private:
  NonTrivial __value_;
};

inline constexpr partial_ordering partial_ordering::less(-1);
inline constexpr partial_ordering partial_ordering::unordered(-2);
inline constexpr partial_ordering partial_ordering::equivalent(0);
inline constexpr partial_ordering partial_ordering::greater(1);

class strong_ordering {
  using _ValueT = signed char;
  explicit constexpr strong_ordering(signed char x) : __value_(x) {}

public:
  static const strong_ordering less;
  static const strong_ordering equal;
  static const strong_ordering equivalent;
  static const strong_ordering greater;

  // Make this type non-trivially copyable
  constexpr strong_ordering(strong_ordering const &__other) : __value_(__other.__value_) {}

private:
  _ValueT __value_;
};

inline constexpr strong_ordering strong_ordering::less(-1);
inline constexpr strong_ordering strong_ordering::equal(0);
inline constexpr strong_ordering strong_ordering::equivalent(0);
inline constexpr strong_ordering strong_ordering::greater(1);

class strong_equality {
  using _ValueT = signed char;
  explicit constexpr strong_equality(signed char x) : __value_(x) {}

public:
  static const strong_equality equal;
  static const strong_equality equivalent;
  static const strong_equality nonequal;
  static const strong_equality nonequivalent;

private:
  NonTrivial __value_;
};

inline constexpr strong_equality strong_equality::equal(0);
inline constexpr strong_equality strong_equality::equivalent(0);
inline constexpr strong_equality strong_equality::nonequal(1);
inline constexpr strong_equality strong_equality::nonequivalent(1);

} // namespace __1
} // namespace std

// CHECK: $_ZNSt3__115strong_orderingC1ERKS0_ = comdat any
// CHECK: $_ZNSt3__116partial_orderingC1ERKS0_ = comdat any
// CHECK: $_ZNSt3__115strong_equalityC1ERKS0_ = comdat any
// CHECK: $_ZNSt3__115strong_orderingC2ERKS0_ = comdat any
// CHECK: $_ZNSt3__116partial_orderingC2ERKS0_ = comdat any

// CHECK-LABEL: @_Z30test_non_trivially_copyable_soii
auto test_non_trivially_copyable_so(int x, int y) {
  static_assert(!__is_trivially_constructible(std::strong_ordering, std::strong_ordering const &), "");
  // CHECK: %cmp.lt = icmp slt i32 %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, %[[SO]]* @[[SO_LT]], %[[SO]]* @[[SO_GT]]
  // CHECK: %cmp.eq = icmp eq i32 %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, %[[SO]]* @[[SO_EQ]], %[[SO]]* %sel.lt
  // CHECK: call void @_ZNSt3__115strong_orderingC1ERKS0_(%[[SO]]* %agg.result, %[[SO]]* dereferenceable(1) %sel.eq)
  // CHECK: ret void
  return x <=> y;
}

// CHECK: define linkonce_odr void @_ZNSt3__115strong_orderingC1ERKS0_

// CHECK-LABEL: @_Z35test_non_trivially_copyable_defaultdd
auto test_non_trivially_copyable_default(double x, double y) {
  static_assert(!__is_trivially_constructible(std::partial_ordering, std::partial_ordering const &), "");
  // CHECK: %cmp.eq = fcmp oeq double %0, %1
  // CHECK: %sel.eq = select i1 %cmp.eq, %[[PO]]* @[[PO_EQ]], %[[PO]]* @[[PO_UNORD]]
  // CHECK: %cmp.gt = fcmp ogt double %0, %1
  // CHECK: %sel.gt = select i1 %cmp.gt, %[[PO]]* @[[PO_GT]], %[[PO]]* %sel.eq
  // CHECK: %cmp.lt = fcmp olt double %0, %1
  // CHECK: %sel.lt = select i1 %cmp.lt, %[[PO]]* @[[PO_LT]], %[[PO]]* %sel.gt
  // CHECK: call void @_ZNSt3__116partial_orderingC1ERKS0_(%[[PO]]* %agg.result, %[[PO]]* dereferenceable(4) %sel.lt)
  // CHECK: ret void
  return x <=> y;
}

// CHECK: define linkonce_odr void @_ZNSt3__116partial_orderingC1ERKS0_

struct ClassTy {};
using MemPtr = void (ClassTy::*)(int);

// CHECK-LABEL: @_Z36test_non_trivially_copyable_implicitM7ClassTyFviES1_
auto test_non_trivially_copyable_implicit(MemPtr x, MemPtr y) {
  // CHECK: %cmp.ptr = icmp eq i64 %lhs.memptr.ptr, %rhs.memptr.ptr
  // CHECK: %cmp.ptr.null = icmp eq i64 %lhs.memptr.ptr, 0
  // CHECK: %cmp.adj = icmp eq i64 %lhs.memptr.adj, %rhs.memptr.adj
  // CHECK: %6 = or i1 %cmp.ptr.null, %cmp.adj
  // CHECK: %memptr.eq = and i1 %cmp.ptr, %6
  // CHECK: %sel.eq = select i1 %memptr.eq, %[[SE]]* @[[SE_EQ]], %[[SE]]* @[[SE_NE]]
  // CHECK: call void @_ZNSt3__115strong_equalityC1ERKS0_(%[[SE]]* %agg.result, %[[SE]]* dereferenceable(4) %sel.eq)
  // CHECK: ret
  return x <=> y;
}

// CHECK: define linkonce_odr void @_ZNSt3__115strong_orderingC2ERKS0_
// CHECK: define linkonce_odr void @_ZNSt3__116partial_orderingC2ERKS0_
// CHECK: define linkonce_odr void @_ZNSt3__115strong_equalityC2ERKS0_
