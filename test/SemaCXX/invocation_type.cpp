// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

static_assert(!__is_identifier(__raw_invocation_type), "");
static_assert(__is_identifier(raw_invocation_type), "");
static_assert(__has_feature(raw_invocation_type), "");

template <class Tp>
Tp &&Declval();

template <class>
struct ReturnType;
template <class Ret, class... Args>
struct ReturnType<Ret(Args...)> {
  using type = Ret;
};
template <class T>
using ReturnT = typename ReturnType<T>::type;

template <class... Args, class = __raw_invocation_type(Args...)>
constexpr bool HasTypeImp(int) { return true; }
template <class... Args>
constexpr bool HasTypeImp(long) { return false; }

template <class... Args>
constexpr bool HasType() { return HasTypeImp<Args...>(0); }

template <class... T>
using RIT = __raw_invocation_type(T...);

template <class>
struct Printer;

#define CHECK_HAS_TYPE(...) static_assert(HasType<__VA_ARGS__>(), "");
#define CHECK_NO_TYPE(...) static_assert(!HasType<__VA_ARGS__>(), "");
#define CHECK_SAME(...) static_assert(__is_same(__VA_ARGS__), "")

namespace CheckParsing {
// expected-error@+2 {{expected a type}}
// expected-error@+1 {{C++ requires a type specifier for all declarations}}
__raw_invocation_type() x;
} // namespace CheckParsing

namespace TestNonCallable {
  static_assert(!HasType<void>(), "");
  static_assert(!HasType<void, void>(), "");
  static_assert(!HasType<int>(), "");
  static_assert(!HasType<int, int>(), "");
  static_assert(!HasType<void *>(), "");
  static_assert(!HasType<void *, int *>(), "");
  enum E {};
  enum class EC : unsigned {};
  static_assert(!HasType<E>(), "");
  static_assert(!HasType<E, int>(), "");
  static_assert(!HasType<EC>(), "");
  static_assert(!HasType<EC, EC>(), "");
} // namespace TestNonCallable

namespace FuncTest {
  using F0 = int();
  using F1 = long(int);
  using F2 = unsigned(void*, const char*);
  CHECK_SAME(RIT<F0>, int());
  CHECK_SAME(RIT<F1, char>, long(int));
  CHECK_SAME(RIT<F2, int*, const char*>, unsigned(void*, const char*));

  // Check arity
  static_assert(HasType<F0>(), "");
  static_assert(!HasType<F0, int>(), "");
  static_assert(!HasType<F0, int, int>(), "");
  static_assert(HasType<F1, int>(), "");
  static_assert(!HasType<F1>(), "");
  static_assert(!HasType<F1, int, int>(), "");

  struct Tag {};
  // test return types
  using FV = Tag();
  using FL = Tag&();
  using FCL = Tag const&();
  using FR = Tag&&();
  using FCR = Tag const&&();
  CHECK_SAME(RIT<FV>, Tag());
  CHECK_SAME(RIT<FL>, Tag&());
  CHECK_SAME(RIT<FCL>, Tag const&());
  CHECK_SAME(RIT<FR>, Tag&&());
  CHECK_SAME(RIT<FCR>, Tag const&&());

  // test function value categories
  using FPure = int(int);
  using FPtr = int(*)(int);
  using FLRef = int(&)(int);
  using FRRef = int(&&)(int);
  CHECK_SAME(RIT<FPure, int>, int(int));
  CHECK_SAME(RIT<FPtr, int>, int(int));
  CHECK_SAME(RIT<FLRef, int>, int(int));
  CHECK_SAME(RIT<FRRef, int>, int(int));

  // test that the actual function return type is returned, and not the type of
  // the call expression.
  using FCI = const int();
  CHECK_SAME(RIT<FCI>, const int());
} // namespace FuncTest

namespace MemFuncTests {
  struct A {};
  struct B : public A {};
  struct C {};
  using AF0 = void (A::*)() const;
  using AF1 = int (A::*)(int);
  using AF2 = int &(A::*)(void *)const;
  using BF0 = char (B::*)();
  using BF1 = long (B::*)(long);
  using BF2 = int && (B::*)(const char *, const char *);

  // expected-error@+1 {{__raw_invocation_type with a pointer to member function requires 2 or more arguments; have 1 argument}}
  using Fail1 = __raw_invocation_type(AF1);

  CHECK_SAME(RIT<AF0, A>, void(A &&));
  CHECK_SAME(RIT<AF0, A &>, void(A &));
  CHECK_SAME(RIT<AF0, A const &>, void(A const &));
  CHECK_SAME(RIT<AF0, A &&>, void(A &&));
  CHECK_SAME(RIT<AF0, B>, void(B &&));
  CHECK_SAME(RIT<AF0, B &>, void(B &));
  CHECK_SAME(RIT<AF0, B const &>, void(B const &));

  // Check arity
  static_assert(HasType<AF0, A &>(), "");
  static_assert(!HasType<AF0, A &, int>(), "");
  static_assert(!HasType<AF0, A &, int, int>(), "");
  static_assert(HasType<AF1, A &, int>(), "");
  static_assert(!HasType<AF1, A &>(), "");
  static_assert(!HasType<AF1, A &, int, int>(), "");

  // Check return type
  struct Tag {};
  using AFV = Tag (A::*)();
  using AFCV = const Tag (A::*)();
  using AFL = Tag &(A::*)();
  using AFR = Tag && (A::*)();
  using AFCL = Tag const &(A::*)();
  using AFCR = Tag const && (A::*)();
  CHECK_SAME(RIT<AFV, A>, Tag(A &&));
  CHECK_SAME(RIT<AFCV, A>, const Tag(A &&));
  CHECK_SAME(RIT<AFL, A>, Tag &(A &&));
  CHECK_SAME(RIT<AFR, A>, Tag && (A &&));
  CHECK_SAME(RIT<AFCL, A>, const Tag &(A &&));
  CHECK_SAME(RIT<AFCR, A>, const Tag && (A &&));

  static_assert(HasType<AF0, A>(), "");
  static_assert(HasType<AF0, B>(), "");
  static_assert(!HasType<AF0, C>(), "");

  static_assert(HasType<BF0, B>(), "");
  static_assert(!HasType<BF0, A>(), "");

  // Test cv qualifiers
  using AN = void (A::*)();
  using AC = void (A::*)() const;
  static_assert(HasType<AN, A &>(), "");
  static_assert(HasType<AC, A &>(), "");
  static_assert(!HasType<AN, const A &>(), "");
  static_assert(HasType<AC, const A &>(), "");

  // Test reference qualifiers
  using AL = void (A::*)() &;
  using AR = void (A::*)() &&;
  static_assert(HasType<AL, A &>(), "");
  static_assert(HasType<AR, A &&>(), "");
  static_assert(!HasType<AL, A &&>(), "");
  static_assert(!HasType<AR, A &>(), "");
} // namespace MemFuncTests

namespace MemObjTest {
  struct A {};
  struct B : public A {};
  struct C {};
  using AF1 = int(A::*);
  using BF1 = long(B::*);

  // expected-error@+1 {{__raw_invocation_type with a pointer to member data requires 2 arguments; have 1 argument}}
  using Fail1 = __raw_invocation_type(AF1);

  // expected-error@+1 {{__raw_invocation_type with a pointer to member data requires 2 arguments; have 3 arguments}}
  using Fail2 = __raw_invocation_type(AF1, A, A);

  template <class MemFn, class Arg, class Expect>
  void test() {
    MemFn ptr = Declval<MemFn>();
    using ResT = decltype(Declval<Arg>().*ptr);
    CHECK_SAME(RIT<MemFn, Arg>, Expect);
    static_assert(__is_same(ReturnT<Expect>, ResT), "");
  };
  template void test<AF1, A, int && (A &&)>();
  template void test<AF1, A &&, int && (A &&)>();
  template void test<AF1, A &, int &(A &)>();
  template void test<AF1, const A &, const int &(A const &)>();
  template void test<AF1, B, int && (B &&)>();
  template void test<AF1, B &, int &(B &)>();

  static_assert(!HasType<AF1, C>(), "");
  static_assert(!HasType<BF1, A>(), "");

  using AFC = const int(A::*);
  template void test<AFC, A &, const int &(A &)>();
  template void test<AFC, const A &, const int &(const A &)>();
  template void test<AFC, A &&, const int && (A &&)>();
} // namespace MemObjTest

namespace ObjectTests {
  struct Callable {
    int operator()(int, int * = nullptr) const;
    long operator()(long) const;
  };

  using Test1 = __raw_invocation_type(Callable, int);

  CHECK_SAME(RIT<Callable, int>, int(int));
  CHECK_SAME(RIT<Callable, int, int *>, int(int, int *));
  CHECK_SAME(RIT<Callable, long>, long(long));
  CHECK_SAME(RIT<int (*)(int), long>, int(int));

  template <int>
  struct Tag {};
  struct CVTest {
    Tag<0> operator()();
    Tag<1> operator()() const;
    Tag<2> operator()() const volatile;
  };
  CHECK_SAME(RIT<CVTest>, Tag<0>());
  CHECK_SAME(RIT<const CVTest>, Tag<1>());
  CHECK_SAME(RIT<const volatile CVTest>, Tag<2>());

  struct RefTest {
    Tag<0> operator()() &;
    Tag<1> operator()() &&;
    Tag<2> operator()() const &;
    Tag<3> operator()() const &&;
  };
  CHECK_SAME(RIT<RefTest &>, Tag<0>());
  CHECK_SAME(RIT<RefTest>, Tag<1>());
  CHECK_SAME(RIT<RefTest &&>, Tag<1>());
  CHECK_SAME(RIT<const RefTest &>, Tag<2>());
  CHECK_SAME(RIT<const RefTest>, Tag<3>());
  CHECK_SAME(RIT<const RefTest &&>, Tag<3>());
} // namespace ObjectTests

namespace VarargsTest {
  struct Obj {
    void operator()(int, int &, ...);
  };
  CHECK_SAME(RIT<Obj, int, int &, int, int &, int const &, int &&>,
             void(int, int &, int, int, int, int));

  using Fn = void(...);
  CHECK_SAME(RIT<Fn, short, unsigned char, int &&, long long, char32_t>,
             void(int, int, int, long long, unsigned int));
} // namespace VarargsTest
