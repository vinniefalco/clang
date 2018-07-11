// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -fresumable-functions %s

resumable int resumable_fn() { return 42; }

struct TestVirtual {
  virtual resumable void foo();
  resumable void bar(); // OK
};

namespace TestBreakResumableScope {

inline void break_resumable() {
  break resumable; // OK
}

} // end namespace TestBreakResumableScope

namespace TestRedecl {

resumable void p1(); // expected-note {{previous declaration is here}}
void p1();           // expected-error {{non-resumable declaration of 'p1' follows resumable declaration}}

void p2();           // expected-note {{previous declaration is here}}
resumable void p2(); // expected-error {{resumable declaration of 'p2' follows non-resumable declaration}}

} // end namespace TestRedecl

struct TestSpecialMember {
  // FIXME(EricWF): Should constructors be resumable?
  resumable TestSpecialMember();  // expected-error{{constructor cannot be declared resumable}}
  resumable ~TestSpecialMember(); // expected-error{{destructor cannot be declared resumable}}
  resumable void operator=(int);  // expected-error{{assignment operator cannot be declared resumable}}
};

struct TestImplicitSpecialMember {
  // expected-error@+2 {{constructor cannot be a resumable function}}
  // expected-note@+1 {{function made resumable by 'break resumable' statement here}}
  TestImplicitSpecialMember() { break resumable; }
  // expected-error@+2 {{destructor cannot be a resumable function}}
  // expected-note@+1 {{function made resumable by 'break resumable' statement here}}
  ~TestImplicitSpecialMember() { break resumable; }
  // expected-error@+2 {{assignment operator cannot be a resumable function}}
  // expected-note@+1 {{function made resumable by call to resumable function 'resumable_fn' here}}
  void operator=(int) { resumable_fn(); }
};

namespace TestTagDecl {

resumable class C1 {};  // expected-error {{class cannot be marked resumable}}
resumable struct S1 {}; // expected-error {{struct cannot be marked resumable}}
resumable union U1 {};  // expected-error {{union cannot be marked resumable}}
resumable enum E1 {};   // expected-error {{enum cannot be marked resumable}}
template <typename T>
resumable class TC1 {}; // expected-error {{class cannot be marked resumable}}
template <typename T>
resumable struct TS1 {}; // expected-error {{struct cannot be marked resumable}}
template <typename T>
resumable union TU1 {}; // expected-error {{union cannot be marked resumable}}
class C2 {
} resumable; // expected-error {{class cannot be marked resumable}}
struct S2 {
} resumable; // expected-error {{struct cannot be marked resumable}}
union U2 {
} resumable;          // expected-error {{union cannot be marked resumable}}
enum E2 {} resumable; // expected-error {{enum cannot be marked resumable}}

resumable int; // expected-error {{resumable can only be used in variable and function declarations}}

} // end namespace TestTagDecl

namespace TestResumableVarDecl {
int foo(); // expected-note {{function declared here}}
resumable int bar() { return 42; }

// expected-error@+1 {{initializer for resumable variable declaration 'x' calls a non-resumable function}}
resumable auto x = foo();
resumable auto y = bar(); // OK

inline int implicitly_resumable() {
  return bar();
}
resumable auto z = implicitly_resumable();

int foo() {
  // expected-error@+1 {{resumable variable declaration 'x' requires an initializer}}
  resumable auto x;
}

} // namespace TestResumableVarDecl

namespace TestAddrOfResumable {

resumable void foo() {}
inline void bar() { foo(); }
inline void baz() { break resumable; }

auto x = &foo; // expected-error {{cannot take address of resumable function 'foo'}}
auto y = &bar; // expected-error {{cannot take address of resumable function 'bar'}}
auto z = &baz; // expected-error {{cannot take address of resumable function 'baz'}}

} // namespace TestAddrOfResumable

namespace TestImplicitlyResumable {

struct Class {
  resumable int foo() { return 42; }
  int bar() {
    break resumable;
    return 1;
  }
  int baz() { return bar(); }

  int operator()() { return baz(); }
  int operator()(int x) { return x; } // expected-note {{declared here}}
};

inline int foo() { return Class{}.foo(); }

auto lam = []() { break resumable; return 42; };
auto lam2 = []() { return 42; }; // expected-note {{function declared here}}

resumable auto t1 = Class{}.foo();
resumable auto t2 = Class{}.bar();
resumable auto t3 = Class{}.baz();
resumable auto t4 = foo();
resumable auto t5 = Class{}();
resumable auto t6 = Class{}(42); // expected-error {{initializer for resumable variable declaration 't6' calls a non-resumable function}}
resumable auto t7 = lam();
resumable auto t8 = lam2(); // expected-error {{initializer for resumable variable declaration 't8' calls a non-resumable function}}

} // namespace TestImplicitlyResumable

namespace TestMixedConstexpr {

constexpr resumable void t1(); // expected-error {{resumable function 't1' declared constexpr}}

// expected-error@+1 {{resumable function 't2' declared constexpr}}
constexpr void t2() {
  break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
}

} // namespace TestMixedConstexpr

namespace TestNonInlineResumable {
// expected-error@+1 {{implicitly resumable function 't1' not declared inline}}
void t1() {
  break resumable; // expected-note{{function made resumable by 'break resumable' statement here}}
}
inline void t2() { break resumable; } // OK.
inline void t3();
void t3() { break resumable; } // OK.
void t4();
inline void t4() { break resumable; } // OK.
static void t5() { break resumable; } // OK?

namespace {
void t6() { break resumable; } // OK?
} // namespace

} // namespace TestNonInlineResumable

namespace TestNoBodyAvailable {

resumable void foo();

resumable void t1() {
  foo(); // expected-error {{resumable function 'foo' is not defined before it is used}}
}
inline void t2() {
  foo(); // expected-error {{resumable function 'foo' is not defined before it is used}}
}

resumable void foo() {}

inline void t3() { foo(); }

} // namespace TestNoBodyAvailable

namespace TestResumableVarSpec {

resumable int foo() { return 42; }

constexpr resumable auto t1 = foo();    // expected-error {{constexpr variable 't1' cannot be declared resumable}}
thread_local resumable auto t2 = foo(); // expected-error {{thread_local variable 't2' cannot be declared resumable}}

static resumable auto t4 = foo(); // OK.

void test_in_fn() {
  // expected-error@+1 {{static variable 't5' cannot be declared resumable}}
  static resumable auto t5 = foo();
  // expected-error@+1 {{thread_local variable 't6' cannot be declared resumable}}
  thread_local resumable auto t6 = foo();
  // expected-error@+2 {{static variable 't7' cannot be declared resumable}}
  // expected-error@+1 {{thread_local variable 't7' cannot be declared resumable}}
  static thread_local resumable auto t7 = foo();
}

struct TestClass {
  // FIXME(EricWF): Should we allow this?
  // expected-error@+1 {{static variable 't8' cannot be declared resumable}}
  static resumable auto t8 = foo();
  // expected-error@+2 {{static variable 't9' cannot be declared resumable}}
  // expected-error@+1 {{resumable variable declaration 't9' requires an initializer}}
  static resumable auto t9;
};

} // namespace TestResumableVarSpec

namespace TestImplicitResumableFunctionSpec {
resumable int foo() { return 42; }

template <class T>
// expected-error@+1 {{resumable function 'foo_templ' declared constexpr}}
inline constexpr int foo_templ() {
  break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
  return 42;
}

// expected-error@+1 {{resumable function 't1' declared constexpr}}
inline constexpr void t1() {
  break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
}

struct TestClass {
  // expected-error@+1 {{virtual function cannot be resumable}}
  virtual void t2() {
    break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
  }
  virtual void t3();
  // expected-error@+1 {{virtual function cannot be resumable}}
  virtual void t4() {
    foo(); // expected-note{{function made resumable by call to resumable function 'foo' here}}
  }
  virtual void t5();
  // expected-error@+1 {{resumable function 't6' declared constexpr}}
  constexpr void t6() const {
    break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
  }
  constexpr void t7() const;
};

// expected-error@+1 {{virtual function cannot be resumable}}
inline void TestClass::t3() {
  break resumable; // expected-note {{function made resumable by 'break resumable' statement here}}
}
// expected-error@+1 {{virtual function cannot be resumable}}
inline void TestClass::t5() {
  foo(); // expected-note{{function made resumable by call to resumable function 'foo' here}}
}

// expected-error@+1 {{resumable function 't7' declared constexpr}}
constexpr void TestClass::t7() const {
  foo(); // expected-note {{function made resumable by call to resumable function 'foo' here}}
}

} // namespace TestImplicitResumableFunctionSpec

namespace TestConstexprLambda {
// FIXME(EricWF): Implement this
}

namespace ExternTemplateTest {

template <class T>
//expected-error@+2 {{explicit instantiation declaration of resumable function 't1<long>'}}}}
//expected-error@+1 {{explicit instantiation definition of resumable function 't1<int>'}}}}
resumable void t1() {
}

// expected-note@+1 {{explicit instantiation declaration is here}}
extern template void t1<long>();

inline void use_t1() {
  t1<long>(); // expected-note{{in instantiation of function template specialization 'ExternTemplateTest::t1<long>' requested here}}
}

// expected-note@+1 {{in instantiation of function template specialization 'ExternTemplateTest::t1<int>' requested here}}
template void t1<int>();

template <class T>
// expected-error@+2 {{explicit instantiation declaration of resumable function 't2<long>'}}
// expected-error@+1 {{explicit instantiation definition of resumable function 't2<int>'}}
inline void t2() { break resumable; }

// expected-note@+1 {{explicit instantiation declaration is here}}
extern template void t2<long>();
inline void use_t2() {
  t2<long>(); // expected-note{{in instantiation of function template specialization 'ExternTemplateTest::t2<long>' requested here}}
}

// expected-note@+1 {{in instantiation of function template specialization 'ExternTemplateTest::t2<int>' requested here}}
template void t2<int>();

template <class T>
struct TestClass {
  // expected-error@+2 {{explicit instantiation declaration of resumable function 't3'}}
  // expected-error@+1 {{explicit instantiation definition of resumable function 't3'}}
  resumable void t3() {}

  // expected-error@+2 {{explicit instantiation declaration of resumable function 't4'}}
  // expected-error@+1 {{explicit instantiation definition of resumable function 't4'}}
  void t4() {
    break resumable;
  }
};

// expected-note@+1 2 {{explicit instantiation declaration is here}}
extern template struct TestClass<long>;

inline void instant_class() {
  TestClass<long>{}.t3(); // expected-note{{in instantiation of member function 'ExternTemplateTest::TestClass<long>::t3' requested here}}
  TestClass<long>{}.t4(); // expected-note{{in instantiation of member function 'ExternTemplateTest::TestClass<long>::t4' requested here}}
}

// expected-note@+2 {{in instantiation of member function 'ExternTemplateTest::TestClass<int>::t3' requested here}}
// expected-note@+1 {{in instantiation of member function 'ExternTemplateTest::TestClass<int>::t4' requested here}}
template struct TestClass<int>;
} // namespace ExternTemplateTest

namespace TestCallInResumableInitializer {
resumable int foo(int x = 42) { return x; }
resumable int other() { return 101; }
int not_res() { return -1; }
resumable bool pred() {
  return true;
}

void t1(bool b) {
  resumable auto x = foo(); // OK.
  for (resumable auto y = foo();;) {
  }
  resumable auto z = b ? foo() : other();
}

// expected-error@+1 {{implicitly resumable function 't2' not declared inline}}
void t2() {
  // expected-note@+1 {{function made resumable by call to resumable function 'other' here}}
  resumable auto x = foo(other()); // Not OK?
}
// expected-error@+1 {{implicitly resumable function 't3' not declared inline}}
void t3() {
  // expected-note@+1 {{function made resumable by call to resumable function 'pred' here}}
  resumable auto x = pred() ? foo() : other();
}
}

namespace test_class_members {
template <class T>
struct IsConst {
  enum { value = false };
};
template <class T>
struct IsConst<const T> {
  enum { value = true };
};

template <int>
struct Tag {};
resumable Tag<1> foo() { return {}; }
resumable Tag<2> bar() noexcept(false) { return {}; }
resumable Tag<3> baz() noexcept { return {}; }
int global = 42;
resumable const int &ref_ty() { return global; }

void test_type_equal() {
  resumable auto r = foo();
  resumable auto r2 = foo();
  static_assert(!__is_same(decltype(r), decltype(r2)), "");
}

void t1() {
  resumable auto r = foo();
  using T = decltype(r);
  using Result = T::result_type;
  static_assert(__is_same(Result, Tag<1>), "");

  resumable auto r2 = ref_ty();
  using T2 = decltype(r2);
  static_assert(__is_same(T2::result_type, int const &), "");
}

void test_ready() {
  resumable auto r = foo();
  const auto &cr = r;
  using T = decltype(r);
  static_assert(__is_same(decltype(r.ready()), bool), "");
  static_assert(noexcept(r.ready()), "");
  bool ready = cr.ready();
}

void test_resume() {
  resumable auto r1 = foo(); // expected-note {{'resume' declared here}}
  resumable auto r2 = bar();
  resumable auto r3 = baz();
  static_assert(!noexcept(r1.resume()), "");
  static_assert(!noexcept(r2.resume()), "");
  static_assert(noexcept(r3.resume()), "");

  static_assert(__is_same(decltype(r1.resume()), void), "");

  r1.resume();

  const auto &cr = r1;
  // expected-error-re@+1 {{'this' argument to member function 'resume' has type 'const (anonymous class at {{.*}}resumable-functions.cpp{{.*}})}}
  cr.resume();
}

void test_result() {
  resumable auto r1 = foo(); // expected-note {{'result' declared here}}
  resumable auto r2 = bar();
  static_assert(!noexcept(r1.result()), "");
  static_assert(__is_same(decltype(r1.result()), Tag<1>), "");
  static_assert(__is_same(decltype(r2.result()), Tag<2>), "");

  Tag<1> t = r1.result();

  const auto &cr = r1;
  // expected-error-re@+1 {{'this' argument to member function 'result' has type 'const (anonymous class at {{.*}}resumable-functions.cpp{{.*}})}}
  cr.result();

  resumable auto r3 = ref_ty();
  static_assert(__is_same(decltype(r3.result()), const int &), "");
  const int &t2 = r3.result();
}

void test_deleted_copy() {
  resumable auto r = foo();
  auto r2 = r;
}

} // namespace test_class_members
