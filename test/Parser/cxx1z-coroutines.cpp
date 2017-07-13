// RUN: %clang_cc1 -std=c++11 -fcoroutines-ts %s -verify

template<typename T, typename U>
U f(T t) {
  co_await t;
  co_yield t;

  1 + co_await t;
  1 + co_yield t; // expected-error {{expected expression}}

  auto x = co_await t;
  auto y = co_yield t;

  for co_await (int x : t) {}
  for co_await (int x = 0; x != 10; ++x) {} // expected-error {{'co_await' modifier can only be applied to range-based for loop}}

  if (t)
    co_return t;
  else
    co_return {t};
}

struct Y {};
struct X { Y operator co_await(); };
struct Z {};
Y operator co_await(Z);

void f(X x, Z z) {
  x.operator co_await();
  operator co_await(z);
}

void operator co_await(); // expected-error {{must have at least one parameter}}
void operator co_await(X, Y, Z); // expected-error {{must be a unary operator}}
void operator co_await(int); // expected-error {{parameter of class or enumeration type}}

template <class T>
void bar(T) {
  co_promise T z = 42;
}

template <class T>
void baz(T) {
  int x = 42;
  co_promise T z = 42; // expected-error {{'co_promise' declaration must be first declaration in the function}}
}

template <class T>
struct test_mem {
  int mem() {
    co_promise promise_int x = 42; // expected-error {{unknown type name 'promise_int'}}
    co_return 42;
  }
};
