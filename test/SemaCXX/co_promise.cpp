// RUN: %clang_cc1 -triple x86_64-apple-darwin9 %s -std=c++14 -fcoroutines-ts -fsyntax-only -Wignored-qualifiers -Wno-error=return-type -verify -fblocks -Wall -Wextra -Wno-error=unreachable-code
#include "Inputs/std-coroutine.h"

using std::experimental::suspend_always;
using std::experimental::suspend_never;

struct awaitable {
  bool await_ready();
  void await_suspend(std::experimental::coroutine_handle<>); // FIXME: coroutine_handle
  void await_resume();
} a;

struct promise_void {
  void get_return_object();
  suspend_always initial_suspend();
  suspend_always final_suspend();
  void return_void();
  void unhandled_exception();
};

struct promise_void_return_value {
  void get_return_object();
  suspend_always initial_suspend();
  suspend_always final_suspend();
  void unhandled_exception();
  void return_value(int);
};

struct VoidTagNoReturn {
  struct promise_type {
    VoidTagNoReturn get_return_object();
    suspend_always initial_suspend();
    suspend_always final_suspend();
    void unhandled_exception();
  };
};

struct VoidTagReturnValue {
  struct promise_type {
    VoidTagReturnValue get_return_object();
    suspend_always initial_suspend();
    suspend_always final_suspend();
    void unhandled_exception();
    void return_value(int);
  };
};

struct VoidTagReturnVoid {
  struct promise_type {
    VoidTagReturnVoid get_return_object();
    suspend_always initial_suspend();
    suspend_always final_suspend();
    void unhandled_exception();
    void return_void();
  };
};

struct promise_float {
  float get_return_object();
  suspend_always initial_suspend();
  suspend_always final_suspend();
  void return_void();
  void unhandled_exception();
};

struct promise_int { // expected-note 1+ {{candidate}}
  promise_int(int val); // expected-note {{candidate}}
  int get_return_object();
  suspend_always initial_suspend();
  suspend_always final_suspend();
  void return_value(int);
  void unhandled_exception();
  int get();
};

int set_own_promise() {
  co_promise promise_int foo(42);
  co_return foo.get();
}

int no_set_own_promise() { // expected-error {{cannot be a coroutine}}
  co_return 42;
}

int set_own_promise_incorrect() {
  co_promise promise_int foo; // expected-error{{no matching constructor for initialization of 'promise_int'}}
  co_return foo.get();
}

template <>
struct std::experimental::coroutine_traits<void> { using promise_type = promise_void; };

void set_foo() {
  // FIXME: Make this deduce to `promise_void`
  co_promise auto p; // expected-error {{requires an initializer}}
}
