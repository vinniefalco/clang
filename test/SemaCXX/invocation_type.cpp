// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

static_assert(__has_builtin(__invocation_type), "");

struct Callable {
  int operator()(int, int * = nullptr);
  long operator()(long);
};

template <class Fn>
struct InvokeType;

template <class Fn, class... Args>
struct InvokeType<Fn(Args...)> {
  using type = __invocation_type<Fn, Args...>;
};

template <class T>
using InvokeT = typename InvokeType<T>::type;

template <class>
struct Printer;
Printer<__invocation_type<Callable, int>> p;

static_assert(__is_same(InvokeT<Callable(int)>, int(int, int *)), "");
