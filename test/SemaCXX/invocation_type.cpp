// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

static_assert(!__is_identifier(__raw_invocation_type), "");

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

template <class... Args>
using InvokeT = __raw_invocation_type(Args...);

template <class... Args, class = __raw_invocation_type(Args...)>
constexpr bool HasTypeImp(int) { return true; }
template <class... Args>
constexpr bool HasTypeImp(long) { return false; }

template <class... Args>
constexpr bool HasType() { return HasTypeImp<Args...>(0); }

template <class>
struct Printer;

struct Callable {
  int operator()(int, int * = nullptr) const;
  long operator()(long) const;
};

int foo(int);
void *foo(const char *, int &);

template <class T>
using Fn = T;

using MemFn = int (Callable::*)(int, void *);

using Test1 = __raw_invocation_type(Callable, int);
static_assert(HasType<int(int *), int *>(), "");
static_assert(HasType<int (*)(int *), int *>(), "");
static_assert(HasType<int (&)(int *), int *>(), "");

static_assert(!HasType<int(int *), int>(), "");

template <class... T>
using RIT = __raw_invocation_type(T...);

#define CHECK_SAME(...) static_assert(__is_same(__VA_ARGS__), "")
CHECK_SAME(RIT<Callable, int>, int(int));
CHECK_SAME(RIT<Callable, int, int *>, int(int, int *));
CHECK_SAME(RIT<Callable, long>, long(long));
CHECK_SAME(RIT<int (*)(int), long>, int(int));
//Printer<RIT<MemFn, Callable&, int, void*>> p;

CHECK_SAME(RIT<MemFn, Callable, int, int *>, int(Callable &&, int, void *));
CHECK_SAME(RIT<MemFn, Callable &&, int, int *>, int(Callable &&, int, void *));
CHECK_SAME(RIT<MemFn, Callable &, int, void *>, int(Callable &, int, void *));

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

namespace MemObjTest {

struct A {};
struct B : public A {};
struct C {};
using AF1 = int(A::*);
using BF1 = long(B::*);
//using AF2 =  (A::*);

// expected-error@+1 {{__raw_invocation_type with a member data pointer requires 2 arguments; have 1 argument}}
using Fail1 = __raw_invocation_type(AF1);

// expected-error@+1 {{__raw_invocation_type with a member data pointer requires 2 arguments; have 3 arguments}}
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
} // namespace MemObjTest
