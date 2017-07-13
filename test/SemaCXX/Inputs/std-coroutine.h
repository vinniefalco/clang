// RUN: %clang_cc1 -triple x86_64-apple-darwin9 %s -std=c++14 -fcoroutines-ts -fsyntax-only -Wignored-qualifiers -Wno-error=return-type -verify -fblocks -Wno-unreachable-code -Wno-unused-value
#ifndef STD_COROUTINE_H
#define STD_COROUTINE_H

namespace std {
namespace experimental {

template <class...>
using void_t = void;

template <class Ret, class = void>
struct coroutine_traits_base {};
template <class Ret>
struct coroutine_traits_base<Ret, void_t<typename Ret::promise_type>> {
  using promise_type = typename Ret::promise_type;
};

template <class Ret, typename... T>
struct coroutine_traits : coroutine_traits_base<Ret> {
};

template <class Promise = void>
struct coroutine_handle {
  static coroutine_handle from_address(void *);
};
template <>
struct coroutine_handle<void> {
  template <class PromiseType>
  coroutine_handle(coroutine_handle<PromiseType>);
  static coroutine_handle from_address(void *);
};

struct suspend_always {
  bool await_ready() { return false; }
  void await_suspend(coroutine_handle<>) {}
  void await_resume() {}
};

struct suspend_never {
  bool await_ready() { return true; }
  void await_suspend(coroutine_handle<>) {}
  void await_resume() {}
};

} // namespace experimental
} // namespace std

#endif // STD_COROUTINE_H
