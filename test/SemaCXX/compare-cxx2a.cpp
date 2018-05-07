// Force x86-64 because some of our heuristics are actually based
// on integer sizes.

// RUN: %clang_cc1 -triple x86_64-apple-darwin -fcxx-exceptions -fsyntax-only -pedantic -verify -Wsign-compare -std=c++2a %s

#include "Inputs/std-compare.h"

#define ASSERT_TYPE(...) static_assert(__is_same(__VA_ARGS__))
#define ASSERT_EXPR_TYPE(Expr, Expect) static_assert(__is_same(decltype(Expr), Expect));

void self_compare() {
  int a;
  int *b = nullptr;

  (void)(a <=> a); // expected-warning {{self-comparison always evaluates to 'std::strong_ordering::equal'}}
  (void)(b <=> b); // expected-warning {{self-comparison always evaluates to 'std::strong_ordering::equal'}}
}

void test0(long a, unsigned long b) {
  enum EnumA : int {A};
  enum EnumB {B};
  enum EnumC {C = 0x10000};

  (void)((short)a <=> (unsigned short)b);

  // (a,b)
  (void)(a <=> (unsigned long)b); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)(a <=> (unsigned int) b);
  (void)(a <=> (unsigned short) b);
  (void)(a <=> (unsigned char) b);
  (void)((long)a <=> b);                // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)a <=> b);                 // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((short)a <=> b);               // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((signed char)a <=> b);         // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((long)a <=> (unsigned long)b); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)a <=> (unsigned int)b);   // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((short) a <=> (unsigned short) b);
  (void)((signed char) a <=> (unsigned char) b);

  (void)(A < 42);
  // (A,b)
  (void)(A <=> (unsigned long) b);
  (void)(A <=> (unsigned int) b);
  (void)(A <=> (unsigned short) b);
  (void)(A <=> (unsigned char) b);
  (void)((long) A <=> b);
  (void)((int) A <=> b);
  (void)((short) A <=> b);
  (void)((signed char) A <=> b);
  (void)((long) A <=> (unsigned long) b);
  (void)((int) A <=> (unsigned int) b);
  (void)((short) A <=> (unsigned short) b);
  (void)((signed char) A <=> (unsigned char) b);

  // (a,B)
  (void)(a <=> (unsigned long) B); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'long' to 'unsigned long'}}
  (void)(a <=> (unsigned int) B);
  (void)(a <=> (unsigned short) B);
  (void)(a <=> (unsigned char) B);
  (void)((long) a <=> B);
  (void)((int) a <=> B);
  (void)((short) a <=> B);
  (void)((signed char) a <=> B);
  (void)((long) a <=> (unsigned long) B); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'long' to 'unsigned long'}}
  (void)((int) a <=> (unsigned int) B); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'int' to 'unsigned int'}}
  (void)((short) a <=> (unsigned short) B);
  (void)((signed char) a <=> (unsigned char) B);

  // (C,b)
  (void)(C <=> (unsigned long) b);
  (void)(C <=> (unsigned int) b);
  (void)(C <=> (unsigned short) b); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'unsigned short' is always 'std::strong_ordering::greater'}}
  (void)(C <=> (unsigned char) b);  // expected-warning {{comparison of constant 'C' (65536) with expression of type 'unsigned char' is always 'std::strong_ordering::greater'}}
  (void)((long) C <=> b);
  (void)((int) C <=> b);
  (void)((short) C <=> b);
  (void)((signed char) C <=> b);
  (void)((long) C <=> (unsigned long) b);
  (void)((int) C <=> (unsigned int) b);
  (void)((short) C <=> (unsigned short) b);
  (void)((signed char) C <=> (unsigned char) b);

  // (a,C)
  (void)(a <=> (unsigned long) C); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'long' to 'unsigned long'}}
  (void)(a <=> (unsigned int) C);
  (void)(a <=> (unsigned short) C);
  (void)(a <=> (unsigned char) C);
  (void)((long) a <=> C);
  (void)((int) a <=> C);
  (void)((short) a <=> C); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'short' is always 'std::strong_ordering::less'}}
  (void)((signed char) a <=> C); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)((long) a <=> (unsigned long) C); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'long' to 'unsigned long'}}
  (void)((int) a <=> (unsigned int) C); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'int' to 'unsigned int'}}
  (void)((short) a <=> (unsigned short) C);
  (void)((signed char) a <=> (unsigned char) C);

  // (0x80000,b)
  (void)(0x80000 <=> (unsigned long) b);
  (void)(0x80000 <=> (unsigned int) b);
  (void)(0x80000 <=> (unsigned short) b); // expected-warning {{result of comparison of constant 524288 with expression of type 'unsigned short' is always 'std::strong_ordering::greater'}}
  (void)(0x80000 <=> (unsigned char) b); // expected-warning {{result of comparison of constant 524288 with expression of type 'unsigned char' is always 'std::strong_ordering::greater'}}
  (void)((long) 0x80000 <=> b);
  (void)((int) 0x80000 <=> b);
  (void)((short) 0x80000 <=> b);
  (void)((signed char) 0x80000 <=> b);
  (void)((long) 0x80000 <=> (unsigned long) b);
  (void)((int) 0x80000 <=> (unsigned int) b);
  (void)((short) 0x80000 <=> (unsigned short) b);
  (void)((signed char) 0x80000 <=> (unsigned char) b);

  // (a,0x80000)
  (void)(a <=> (unsigned long)0x80000); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)(a <=> (unsigned int) 0x80000);
  (void)(a <=> (unsigned short) 0x80000);
  (void)(a <=> (unsigned char) 0x80000);
  (void)((long) a <=> 0x80000);
  (void)((int) a <=> 0x80000);
  (void)((short) a <=> 0x80000); // expected-warning {{comparison of constant 524288 with expression of type 'short' is always 'std::strong_ordering::less'}}
  (void)((signed char) a <=> 0x80000); // expected-warning {{comparison of constant 524288 with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)((long)a <=> (unsigned long)0x80000); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)a <=> (unsigned int)0x80000);   // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((short) a <=> (unsigned short) 0x80000);
  (void)((signed char) a <=> (unsigned char) 0x80000);
}

void test5(bool b, bool b2) {
  enum EnumA { A };
  (void)(b <=> b2);      // OK
  (void)(true <=> b);    // OK
  (void)(b <=> -10);     // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
  (void)(b <=> char(1)); // expected-error {{invalid operands to binary expression ('bool' and 'char')}}
  (void)(b <=> A);       // expected-error {{invalid operands to binary expression ('bool' and 'EnumA')}}

  // FIXME: Should this be accepted when narrowing doesn't occur?
  (void)(b <=> 0); // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
  (void)(b <=> 1); // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
}

void test6(signed char sc) {
  (void)(sc <=> 200); // expected-warning{{comparison of constant 200 with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)(200 <=> sc); // expected-warning{{comparison of constant 200 with expression of type 'signed char' is always 'std::strong_ordering::greater'}}
}

// Test many signedness combinations.
void test7(unsigned long other) {
  // Common unsigned, other unsigned, constant unsigned
  (void)((unsigned)other <=> (unsigned long)(0x1'ffff'ffff)); // expected-warning{{less}}
  (void)((unsigned)other <=> (unsigned long)(0xffff'ffff));
  (void)((unsigned long)other <=> (unsigned)(0x1'ffff'ffff));
  (void)((unsigned long)other <=> (unsigned)(0xffff'ffff));

  // Common unsigned, other signed, constant unsigned
  (void)((int)other <=> (unsigned long)(0xffff'ffff'ffff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)other <=> (unsigned long)(0x0000'0000'ffff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)other <=> (unsigned long)(0x0000'0000'0fff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}}
  (void)((int)other <=> (unsigned)(0x8000'0000));                // expected-error {{argument to 'operator<=>' cannot be narrowed}}

  // Common unsigned, other unsigned, constant signed
  (void)((unsigned long)other <=> (int)(0xffff'ffff)); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned long'}}

  // Common unsigned, other signed, constant signed
  // Should not be possible as the common type should also be signed.

  // Common signed, other signed, constant signed
  (void)((int)other <=> (long)(0xffff'ffff));           // expected-warning{{less}}
  (void)((int)other <=> (long)(0xffff'ffff'0000'0000)); // expected-warning{{greater}}
  (void)((int)other <=> (long)(0x0fff'ffff));
  (void)((int)other <=> (long)(0xffff'ffff'f000'0000));

  // Common signed, other signed, constant unsigned
  (void)((int)other <=> (unsigned char)(0xffff));
  (void)((int)other <=> (unsigned char)(0xff));

  // Common signed, other unsigned, constant signed
  (void)((unsigned char)other <=> (int)(0xff));
  (void)((unsigned char)other <=> (int)(0xffff));  // expected-warning{{less}}

  // Common signed, other unsigned, constant unsigned
  (void)((unsigned char)other <=> (unsigned short)(0xff));
  (void)((unsigned char)other <=> (unsigned short)(0x100)); // expected-warning{{less}}
  (void)((unsigned short)other <=> (unsigned char)(0xff));
}

void test8(void *vp, const void *cvp, int *ip) {
  (void)(vp <=> cvp); // OK, void* comparisons are allowed.
  (void)(vp <=> ip);
  (void)(ip <=> cvp);
}

void test9(long double ld, double d, float f, int i, long long ll) {
  (void)(f <=> ll); // OK, floating-point to integer is OK
  (void)(d <=> ld);
  (void)(i <=> f);
}

typedef int *INTPTR;
void test_typedef_bug(int *x, INTPTR y) {
  (void)(x <=> y);
}

using nullptr_t = decltype(nullptr);

struct Class {};
struct ClassB : Class {};
struct Class2 {};
using FnTy = void(int);
using FnTy2 = long(int);
using MemFnTy = void (Class::*)() const;
using MemFnTyB = void (ClassB::*)() const;
using MemFnTy2 = void (Class::*)();
using MemFnTy3 = void (Class2::*)() const;
using MemDataTy = long(Class::*);
template <class>
struct Printer;
void test_nullptr(int *x, FnTy *fp, MemFnTy memp, MemDataTy memdp) {
  auto r1 = (nullptr <=> nullptr);
  ASSERT_EXPR_TYPE(r1, std::strong_equality);

  auto r2 = (nullptr <=> x);

  ASSERT_EXPR_TYPE(r2, std::strong_equality);

  auto r3 = (fp <=> nullptr);
  ASSERT_EXPR_TYPE(r3, std::strong_equality);

  auto r4 = (0 <=> fp);
  ASSERT_EXPR_TYPE(r4, std::strong_equality);

  auto r5 = (nullptr <=> memp);
  ASSERT_EXPR_TYPE(r5, std::strong_equality);

  auto r6 = (0 <=> memdp);
  ASSERT_EXPR_TYPE(r6, std::strong_equality);

  auto r7 = (0 <=> nullptr);
  ASSERT_EXPR_TYPE(r7, std::strong_equality);
}

void test_compatible_pointer(FnTy *f1, FnTy2 *f2, MemFnTy mf1, MemFnTyB mfb,
                             MemFnTy2 mf2, MemFnTy3 mf3) {
  (void)(f1 <=> f2); // expected-error {{distinct pointer types}}

  auto r1 = (mf1 <=> mfb); // OK
  ASSERT_EXPR_TYPE(r1, std::strong_equality);
  ASSERT_EXPR_TYPE((mf1 <=> mfb), std::strong_equality);

  (void)(mf1 <=> mf2); // expected-error {{distinct pointer types}}
  (void)(mf3 <=> mf1); // expected-error {{distinct pointer types}}
}

// Test that variable narrowing is deferred for value dependent expressions
template <int Val>
auto test_template_overflow() {
  // expected-error@+1 {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned long'}}
  return (Val <=> (unsigned long)0);
}
template auto test_template_overflow<0>();
template auto test_template_overflow<-1>(); // expected-note {{requested here}}

void test_enum_integral_compare() {
  enum EnumA : int {A, ANeg = -1, AMax = __INT_MAX__};
  enum EnumB : unsigned {B, BMax = __UINT32_MAX__ };
  enum EnumC : int {C = -1, C0 = 0};

  (void)(A <=> C); // expected-error {{invalid operands to binary expression ('EnumA' and 'EnumC')}}

  (void)(A <=> (unsigned)0);
  (void)((unsigned)0 <=> A);
  (void)(ANeg <=> (unsigned)0); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned int'}}
  (void)((unsigned)0 <=> ANeg); // expected-error {{cannot be narrowed}}

  (void)(B <=> 42);
  (void)(42 <=> B);
  (void)(B <=> (unsigned long long)42);
  (void)(B <=> -1); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned int'}}
  (void)(BMax <=> (unsigned long)-1);

  (void)(C0 <=> (unsigned)42);
  (void)(C <=> (unsigned)42); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned int'}}
}

namespace EnumCompareTests {

enum class EnumA { A, A2 };
enum class EnumB { B };
enum class EnumC : unsigned { C };

void test_enum_enum_compare_no_builtin() {
  auto r1 = (EnumA::A <=> EnumA::A2); // OK
  ASSERT_EXPR_TYPE(r1, std::strong_ordering);
  (void)(EnumA::A <=> EnumA::A); // expected-warning {{self-comparison always evaluates to 'std::strong_ordering::equal'}}
  (void)(EnumA::A <=> EnumB::B); // expected-error {{invalid operands to binary expression ('EnumCompareTests::EnumA' and 'EnumCompareTests::EnumB')}}
  (void)(EnumB::B <=> EnumA::A); // expected-error {{invalid operands}}
}

template <int>
struct Tag {};
std::strong_ordering operator<=>(EnumA, EnumA) {
  return std::strong_ordering::equal;
}
// expected-note@+1 {{candidate function}},
std::strong_ordering operator<=>(EnumA a, EnumB b) {
  return ((int)a <=> (int)b);
}

void test_enum_ovl_provided() {
  auto r1 = (EnumA::A <=> EnumA::A);
  ASSERT_EXPR_TYPE(r1, std::strong_ordering);
  auto r2 = (EnumA::A <=> EnumB::B);
  ASSERT_EXPR_TYPE(r2, std::strong_ordering);
  (void)(EnumB::B <=> EnumA::A); // OK, chooses reverse order synthesized candidate.
  (void)(EnumB::B <=> EnumC::C); // expected-error {{invalid operands to binary expression ('EnumCompareTests::EnumB' and 'EnumCompareTests::EnumC')}}
}

void enum_float_test() {
  enum EnumA { A };
  (void)(A <=> (float)0);       // expected-error {{invalid operands to binary expression ('EnumA' and 'float')}}
  (void)((double)0 <=> A);      // expected-error {{invalid operands to binary expression ('double' and 'EnumA')}}
  (void)((long double)0 <=> A); // expected-error {{invalid operands to binary expression ('long double' and 'EnumA')}}
}

enum class Bool1 : bool { Zero,
                          One };
enum Bool2 : bool { B2_Zero,
                    B2_One };

void test_bool_enum(Bool1 A1, Bool1 A2, Bool2 B1, Bool2 B2) {
  (void)(A1 <=> A2);
  (void)(B1 <=> B2);
}

} // namespace EnumCompareTests

namespace TestUserDefinedConvSeq {

template <class T, T Val>
struct Conv {
  constexpr operator T() const { return Val; }
  operator T() { return Val; }
};

void test_user_conv() {
  {
    using C = Conv<int, 0>;
    C c;
    const C cc;
    (void)(0 <=> c);
    (void)(c <=> -1);
    (void)((unsigned)0 <=> cc);
    (void)((unsigned)0 <=> c); // expected-error {{argument to 'operator<=>' cannot be narrowed from type 'int' to 'unsigned int'}}
  }
  {
    using C = Conv<int, -1>;
    C c;
    const C cc;
    (void)(c <=> 0);
    (void)(cc <=> (unsigned)0); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned int'}}
    (void)(c <=> (unsigned)0);  // expected-error {{cannot be narrowed from type 'int' to 'unsigned int'}}
  }
}

} // namespace TestUserDefinedConvSeq

void test_array_conv() {
  int arr[5];
  int *ap = arr + 2;
  int arr2[3];
  (void)(arr <=> arr); // expected-error {{invalid operands to binary expression ('int [5]' and 'int [5]')}}
  (void)(+arr <=> arr);
}

void test_mixed_float_int(float f, double d, long double ld) {
  extern int i;
  extern unsigned u;
  extern long l;
  extern short s;
  extern unsigned short us;
  auto r1 = (f <=> i);
  ASSERT_EXPR_TYPE(r1, std::partial_ordering);

  auto r2 = (us <=> ld);
  ASSERT_EXPR_TYPE(r2, std::partial_ordering);

  auto r3 = (s <=> f);
  ASSERT_EXPR_TYPE(r3, std::partial_ordering);

  auto r4 = (0.0 <=> i);
  ASSERT_EXPR_TYPE(r4, std::partial_ordering);
}

namespace NullptrTest {
using nullptr_t = decltype(nullptr);
void foo(nullptr_t x, nullptr_t y) {
  auto r = x <=> y;
  ASSERT_EXPR_TYPE(r, std::strong_equality);
}
} // namespace NullptrTest

namespace ComplexTest {

enum class StrongE {};
enum WeakE { E_One,
             E_Two };

void test_diag(_Complex int ci, _Complex float cf, _Complex double cd, int i, float f, StrongE E1, WeakE E2, int *p) {
  (void)(ci <=> (_Complex int &)ci);
  (void)(ci <=> cf);
  (void)(ci <=> i);
  (void)(ci <=> f);
  (void)(cf <=> i);
  (void)(cf <=> f);
  (void)(ci <=> p); // expected-error {{invalid operands}}
  (void)(ci <=> E1); // expected-error {{invalid operands}}
  (void)(E2 <=> cf); // expected-error {{invalid operands}}
}

void test_int(_Complex int x, _Complex int y) {
  auto r = x <=> y;
  ASSERT_EXPR_TYPE(r, std::strong_equality);
}

void test_double(_Complex double x, _Complex double y) {
  auto r = x <=> y;
  ASSERT_EXPR_TYPE(r, std::weak_equality);
}

} // namespace ComplexTest

namespace TestRewritting {

struct T {
  int x;
  // expected-note@+1 {{candidate}}
  constexpr std::strong_ordering operator<=>(T y) const {
    return (x <=> y.x);
  }
};

struct U {
  int x;
  // FIXME: This diagnostic is wrong-ish.
  // expected-note@+1 {{candidate function not viable: requires single argument 'y', but 2 arguments were provided}}
  constexpr std::strong_equality operator<=>(T y) const {
    if (x == y.x)
      return std::strong_equality::equal;
    return std::strong_equality::nonequal;
  }
};

struct X { int x; };
struct Y { int x; };
template <int>
struct Tag {};
// expected-note@+1 2 {{candidate}}
Tag<0> operator<=>(X, Y) {
  return {};
}
// expected-note@+1 2 {{candidate}}
constexpr auto operator<=>(Y y, X x) {
  return y.x <=> x.x;
}

void foo() {
  T t{42};
  T t2{0};
  U u{101};
  auto r1 = (t <=> u);
  ASSERT_EXPR_TYPE(r1, std::strong_equality);
  auto r2 = (t <=> t2);
  ASSERT_EXPR_TYPE(r2, std::strong_ordering);

  auto r3 = t == u;
  ASSERT_EXPR_TYPE(r3, bool);

  (void)(t < u); // expected-error {{invalid operands to binary expression ('TestRewritting::T' and 'TestRewritting::U')}}

  constexpr X x{1};
  constexpr Y y{2};
  constexpr auto r4 = (y < x);
  static_assert(r4 == false);
  constexpr auto r5 = (x < y);
  static_assert(r5 == true);
}

} // namespace TestRewritting

// The implicit object parameter is not considered when performing partial
// ordering. That makes the reverse synthesized candidates ambiguous with the
// rewritten candidates if any of them resolve to a member function.
namespace TestOvlMatchingIgnoresImplicitObject {
struct U;
// expected-note@+2 {{candidate}}
struct T {
  std::strong_ordering operator<=>(U const &RHS) const;
};
// expected-note@+2 {{candidate}}
struct U {
  std::strong_ordering operator<=>(T const &RHS) const;
};

struct V {
  int x;
};
auto operator<=>(V const &LHS, V &&RHS) { // expected-note 4 {{candidate}}
  return LHS.x <=> RHS.x;
}
auto operator<(V const &, V &&) { // expected-note {{candidate}}
  return std::strong_equality::equal;
}

void test() {
  // expected-error@+1 {{use of overloaded operator '<' is ambiguous}}
  (void)(T{} < U{});
  // expected-error@+1 {{use of overloaded operator '<' is ambiguous}}
  (void)(V{} < V{});
  // expected-error@+1 {{use of overloaded operator '<=>' is ambiguous}}
  (void)(V{} <=> V{});
}

} // namespace TestOvlMatchingIgnoresImplicitObject

namespace TestRewrittenTemplate {

template <class T>
auto test(T const &LHS, T const &RHS) {
  // expected-error@+1 {{invalid operands to binary expression ('const TestRewrittenTemplate::None'}}
  return LHS < RHS;
}
struct None {};
template auto test<None>(None const &, None const &); // expected-note {{requested here}}

struct Relational {};
bool operator<(Relational, Relational);
template auto test<Relational>(Relational const &, Relational const &);

struct ThreeWay {};
std::strong_ordering operator<=>(ThreeWay, ThreeWay);
template auto test<ThreeWay>(ThreeWay const &, ThreeWay const &);

} // namespace TestRewrittenTemplate

namespace IllFormedSpecialMemberTest {

struct T1 {
  // expected-error@+1 {{the parameter for this explicitly-defaulted comparison operator must have type 'const IllFormedSpecialMemberTest::T1 &', but parameter has type 'IllFormedSpecialMemberTest::T1'}}
  void operator<=>(T1) const = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must have type}}
  void operator<=>(T1 &) const = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must have type}}
  void operator<=>(int) const = default;
  // expected-error@+1 {{overloaded 'operator<=>' must be a binary operator (has 1 parameter)}}
  void operator<=>() const = default;
  // expected-error@+1 {{overloaded 'operator<=>' must be a binary operator (has 3 parameters)}}
  void operator<=>(T1 const &, T1 const &) = default;

  // expected-error@+2 {{an explicitly-defaulted comparison operator cannot be a template}}
  template <class U>
  void operator<=>(U const &) = default;

  // FIXME(EricWF): This should delete itself.
  void operator<=>(T1 const &) const = default; // OK
};
// expected-error@+1 {{definition of explicitly defaulted comparison operator}}
void T1::operator<=>(T1 const &) const = default;

struct T2 {
  // expected-error@+1 {{overloaded 'operator<=>' must be a binary operator (has 1 parameter)}}
  friend void operator<=>(T2 const &) = default;
  // expected-error@+1 {{overloaded 'operator<=>' must be a binary operator (has 3 parameters)}}
  friend void operator<=>(T2 const &, T2 const &, T2 const &) = default;

  // FIXME(EricWF): Improve this diagnostic to point to the parameter.
  // expected-error@+2 {{explicitly-defaulted comparison operator must have type}}
  // expected-error@+1 {{explicitly-defaulted comparison operator must have type}}
  friend void operator<=>(T2, T2) = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must have type}}
  friend void operator<=>(T2 &, T2 const &) = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must have type}}
  friend void operator<=>(T2 const &, T2 const volatile &) = default;

  friend auto operator<=>(T2 const &, T2 const &) = default; //OK

  // expected-error@+2 {{an explicitly-defaulted comparison operator cannot be a template}}
  template <class Tp>
  friend void operator<=>(Tp const &, Tp const &) = default;
};

struct T3;
// expected-note@+1 {{previous declaration is here}}
auto operator<=>(T3 const &, T3 const &);
struct T3 {
  // expected-error@+1 {{defaulted definition must be first declaration}}
  friend auto operator<=>(T3 const &, T3 const &) = default;
};

struct T4 {
  bool operator<(T4 const &) const = default; // expected-note {{explicitly defaulted function was implicitly deleted here}}
  bool operator>(T4 const &) const = default;
  bool operator==(T4 const &) const = default;
  bool operator!=(T4 const &) const = default;

  friend bool operator<(T4 const &, T4 const &) = default;
  friend bool operator!=(T4 const &, T4 const &) = default;

  // expected-error@+1 {{explicitly-defaulted comparison operator must return 'bool'}}
  int operator<=(T4 const &) const = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must return 'bool'}}
  bool &operator>=(T4 const &) const = default;
  // expected-error@+1 {{explicitly-defaulted comparison operator must return 'bool'}}
  friend void operator<=(T4 const &, T4 const &) = default;
};
using FriendFn = bool (*)(T4 const &, T4 const &);

auto R1 = &T4::operator<; // expected-error {{attempt to use a deleted function}}

struct T5 {
  bool operator<(T5 const &) const = default; // expected-note {{deleted here}}
  bool operator==(T5 const &) const = default;
  friend std::strong_equality operator<=>(T5 const &, T5 const &) = default;
};
auto R3 = &T5::operator<;  // expected-error {{attempt to use a deleted function}}
auto R4 = &T5::operator==; // OK

// expected-error@+1 {{definition of explicitly defaulted comparison operator}}
bool T5::operator<(T5 const &) const {
  return false;
}

struct T6 {
private:
  auto operator<=>(T6 const &) const = default;
};
struct T7 : T6 {
  bool operator<(T7 const &) const = default; // expected-note {{deleted here}}
};
auto R5 = &T7::operator<; // expected-error {{attempt to use a deleted function}}

struct T8 {
private:
  auto operator<=>(T8 const &) const = default;

public:
  bool operator<(T8 const &) const = default;
};
auto R6 = &T8::operator<; // OK

// FIXME(EricWF): Should T10's defaulted comparison operator be deleted here?
// We use the user defined conversion to 'int', but it's not accessible. This
// results in diagnostics when synthesizing the definition.
struct T9 {
private:
  operator int() const;
};
struct T10 : T9 {
  bool operator<(T10 const &) const = default;
};
auto R7 = &T10::operator<;

} // namespace IllFormedSpecialMemberTest

namespace TestCommonCategoryType {
using SO = std::strong_ordering;
using WO = std::weak_ordering;
using PO = std::partial_ordering;
using SE = std::strong_equality;
using WE = std::weak_equality;

template <class T>
struct Tag { using type = T; };
template <class Tp>
struct CmpResult {
  friend Tp operator<=>(CmpResult const &, CmpResult const &);
};
template <class T>
struct CmpResult<Tag<T>> {
  friend T operator<=>(CmpResult const &, CmpResult const &);
};
template <>
struct CmpResult<Tag<int>> {
  friend auto operator<=>(CmpResult const &, CmpResult const &) = delete;
};

template <class... Ops>
struct Cmp : CmpResult<Ops>... {
  friend auto operator<=>(Cmp const &, Cmp const &) = default;
};

template <class... Ops>
struct Cmp2 : CmpResult<Ops>... {
  auto operator<=>(Cmp2 const &) const = default;
};

void test() {
  {
    using C = Cmp<>;
    C c1, c2;
    ASSERT_EXPR_TYPE(c1 <=> c2, SO);
  }
  {
    using C = Cmp<SO>;
    C c1, c2;
    (void)(c1 <=> c2);
    ASSERT_EXPR_TYPE(c1 <=> c2, SO);
  }
  {
    using C = Cmp2<SO>;
    C c1, c2;
    (void)(c1 <=> c2);
    ASSERT_EXPR_TYPE(c1 <=> c2, SO);
  }
}

} // namespace TestCommonCategoryType

namespace DefaultTest {
struct T {
  int x;
  std::strong_ordering operator<=>(T const &) const { return std::strong_ordering::equal; }
};
struct U {
  float y;
  friend std::partial_ordering operator<=>(U const &, U const &) = default;
};
struct TU : public T, public U {
  friend auto operator<=>(TU const &, TU const &) = default;
};

void test1(T x, T y) {
  (void)(x <=> y);
}
} // namespace DefaultTest

namespace TestRecursion {

template <class Tp>
struct U {
  Tp &t;
  auto operator<=>(U const &) const = default;
};

struct T {
  U<T> u;
  // FIXME(EricWF): Is this diagnostic helpful?
  // expected-note@+1 {{explicitly defaulted function was implicitly deleted here}}
  auto operator<=>(T const &) const = default;
};

void test(T &t1, T &t2) {
  // expected-error@+1 {{object of type 'TestRecursion::T' cannot be compared because its comparison operator is implicitly deleted}}
  using R = decltype(t1 <=> t2);
}

} // namespace TestRecursion

namespace AutoDeductionTest {
template <class Tp>
struct U {
  Tp &t;
  auto operator<=>(U const &u) const {
    return t <=> u.t;
  }
};

struct T {
  U<float> u;
  auto operator<=>(T const &) const = default;
};

void test(T &t1, T &t2) {
  using R = decltype(t1 <=> t2);
  ASSERT_TYPE(R, std::partial_ordering);
}
} // namespace AutoDeductionTest

namespace RecursionTest3 {
template <class Tp>
struct U {
  bool b;
  Tp &t;
  float f;
  auto operator<=>(U const &u) const {
    if (b)
      return t <=> u.t;
    // expected-error@+1 {{'auto' in return type deduced as 'std::__1::partial_ordering' here but deduced as 'std::__1::strong_ordering' in earlier return statement}}
    return f <=> u.f;
  }
};

struct T {
  U<int> u;
  // expected-note@+1 {{in instantiation of member function}}
  auto operator<=>(T const &) const = default;
};

// FIXME(EricWF): Why isn't this working?
void test(T &t1, T &t2) {
  using R = decltype(t1 <=> t2);
}
} // namespace RecursionTest3

namespace ConstexprDeclTest {
struct T {
  // non-constexpr
  std::strong_ordering operator<=>(T const &) const { return std::strong_ordering::equal; }
};

struct U {
  T t;
  // expected-error@+1 {{defaulted definition of comparison operator is not constexpr}}
  constexpr auto operator<=>(U const &) const = default;
};

struct V {
  T t;
  // expected-note@+1 {{declared here}}
  friend auto operator<=>(V const &, V const &) = default;
};
// expected-error@+2 {{must be initialized by a constant expression}}
// expected-note@+1 {{non-constexpr function 'operator<=>' cannot be used in a constant expression}}
constexpr auto R1 = (V{} <=> V{});

} // namespace ConstexprDeclTest

namespace AccessTest {
struct T {
  T() : x(0), y(0), z(0) {}
  T(int x, int y, int z) : x(x), y(y), z(z) {}
  int x;

protected:
  int y;

private:
  int z;

public:
  auto operator<=>(T const &) const = default;
};
struct U {
  U() : x(0), y(0), z(0) {}
  U(int x, int y, int z) : x(x), y(y), z(z) {}
  int x;

protected:
  int y;

private:
  float z;

public:
  auto operator<=>(U const &) const = default;
};

void test() {
  T t1, t2;
  (void)(t1 <=> t2);
  {
    U u1, u2;
    auto r = (u1 <=> u2);
    ASSERT_EXPR_TYPE(r, std::partial_ordering);
  }
}

struct P {
  float x;

private:
  auto operator<=>(P const &) const = default;
};

struct D : P {
  // expected-note@+1 {{explicitly defaulted function was implicitly deleted here}}
  auto operator<=>(D const &) const = default;
};
void test2() {
  D d1, d2;
  // expected-error@+1 {{object of type 'AccessTest::D' cannot be compared because its comparison operator is implicitly deleted}}
  auto r = (d1 <=> d2);
}

} // namespace AccessTest

namespace FieldTests {
struct X {
  X() { x = 0, y = 0; }
  int x : 2;
  int y : 3;
  auto operator<=>(X const &) const = default;
};
void test() {
  X x1, x2;
  auto r = x1 <=> x2;
}

struct Vol {
  std::strong_ordering operator<=>(Vol const &) const volatile;
};
struct Vol2 : Vol {
  auto operator<=>(Vol2 const &) const = default;
};
} // namespace FieldTests

namespace CachedLookupTest {
struct T {};

struct U {
  T t;
  // expected-note@+1 {{implicitly deleted here}}
  auto operator<=>(U const &) const = default;
};
// expected-error@+1 {{comparison operator is implicitly deleted}}
auto r1 = U{} <=> U{};

std::strong_ordering operator<=>(T const &, T const &);

struct V {
  T t;
  auto operator<=>(V const &) const = default;
};
auto r2 = V{} <=> V{};
} // namespace CachedLookupTest

namespace ConstexprDeductionTest {
struct T0 {
  int x;
  constexpr operator int() const { return x; }
};
struct T1 : T0 {
  friend constexpr auto operator<=>(T1 const &, T1 const &) = default;
};
} // namespace ConstexprDeductionTest
