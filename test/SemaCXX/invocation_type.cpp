// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

static_assert(!__is_identifier(__raw_invocation_type), "");

struct Callable {
  int operator()(int, int * = nullptr);
  long operator()(long);
};

int foo(int);
void *foo(const char *, int &);

template <class Fn>
struct InvokeType;

template <class Fn, class... Args>
struct InvokeType<Fn(Args...)> {
  using type = __raw_invocation_type(Fn, Args...);
};

template <class T>
using InvokeT = typename InvokeType<T>::type;

template <class>
struct Printer;
//Printer<__invocation_type<Callable, int>> p;

template <class T>
using Fn = T;

using MemFn = int (Callable::*)(int, void *);

using Test1 = __raw_invocation_type(Callable, int);
//Printer<InvokeT<Callable(int)>> p;

static_assert(__is_same(InvokeT<Callable(int)>, int(int, int *)), "");
static_assert(__is_same(InvokeT<Fn<int (*)(int)>(void *)>, int(int)), "");
static_assert(__is_same(InvokeT<MemFn(Callable &, int, void *)>, int(Callable &, int)), "");
