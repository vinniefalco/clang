// RUN: %clang_cc1 -triple %itanium_abi_triple -ffreestanding -fsyntax-only -verify -std=c++11 %s

#include "limits.h"

template<typename T, typename U>
struct is_same_type {
  static const bool value = false;
};
template <typename T>
struct is_same_type<T, T> {
  static const bool value = true;
};
// expected-error@+1 {{C++ requires a type}}
__underlying_type() x;         // expected-error {{type trait requires 1 argument; have 0 arguments}}
// expected-error@+1 {{C++ requires a type}}
__underlying_type(int, int) y; // expected-error {{type trait requires 1 argument; have 2 arguments}}
__underlying_type(int) a; // expected-error {{only enumeration types}}
__underlying_type(struct b) c; // expected-error {{only enumeration types}}

enum class f : char;
static_assert(is_same_type<char, __underlying_type(f)>::value,
              "f has the wrong underlying type");

enum g {d = INT_MIN };
static_assert(is_same_type<int, __underlying_type(g)>::value,
              "g has the wrong underlying type");

__underlying_type(f) h;
static_assert(is_same_type<char, decltype(h)>::value,
              "h has the wrong type");

template <class ...Args> struct TypeList {};

template <class ...Args>
struct TestParse;

template <class... Args1, class ...Args2>
struct TestParse<TypeList<Args1...>, TypeList<Args2...>> {
  // expected-error@+2 {{type trait requires 1 argument; have 0 arguments}}
  // expected-error@+1 {{type trait requires 1 argument; have 2 arguments}}
  using type = __underlying_type(Args1..., Args2...);
};
static_assert(is_same_type<TestParse<TypeList<f>, TypeList<>>::type, char>::value, "wrong type");
template struct TestParse<TypeList<>, TypeList<>>; // expected-note {{requested here}}
template struct TestParse<TypeList<f, f>, TypeList<>>; // expected-note {{requested here}}


template <typename T>
struct underlying_type {
  typedef __underlying_type(T) type; // expected-error {{only enumeration types}}
};

static_assert(is_same_type<underlying_type<f>::type, char>::value,
              "f has the wrong underlying type in the template");

underlying_type<int>::type e; // expected-note {{requested here}}

using uint = unsigned;
enum class foo : uint { bar };
 
static_assert(is_same_type<underlying_type<foo>::type, unsigned>::value,
              "foo has the wrong underlying type");

namespace PR19966 {
  void PR19966(enum Invalid) { // expected-note 2{{forward declaration of}}
    // expected-error@-1 {{ISO C++ forbids forward references to 'enum'}}
    // expected-error@-2 {{variable has incomplete type}}
    __underlying_type(Invalid) dont_crash;
    // expected-error@-1 {{cannot determine underlying type of incomplete enumeration type 'PR19966::Invalid'}}
  }
  enum E { // expected-note {{forward declaration of 'E'}}
    a = (__underlying_type(E)){}
    // expected-error@-1 {{cannot determine underlying type of incomplete enumeration type 'PR19966::E'}}
    // expected-error@-2 {{constant expression}}
  };
}

template<typename T> void f(__underlying_type(T));
template<typename T> void f(__underlying_type(T));
enum E {};
void PR26014() { f<E>(0); } // should not yield an ambiguity error.

template<typename ...T> void f(__underlying_type(T) v); // expected-error {{declaration type contains unexpanded parameter pack 'T'}}

namespace PR23421 {
template <class T>
using underlying_type_t = __underlying_type(T);
// Should not crash.
template <class T>
struct make_unsigned_impl { using type = underlying_type_t<T>; };
using AnotherType = make_unsigned_impl<E>::type;

// also should not crash.
template <typename T>
__underlying_type(T) ft();
auto x = &ft<E>;
}
