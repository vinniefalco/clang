// RUN: %clang_cc1 -fsyntax-only -verify -fcxx-exceptions %s
// RUN: %clang_cc1 -fsyntax-only -verify -fcxx-exceptions -std=c++14 %s
//
// Tests for "expression traits" intrinsics such as __is_lvalue_expr.
//

#if !__has_feature(cxx_static_assert)
# define CONCAT_(X_, Y_) CONCAT1_(X_, Y_)
# define CONCAT1_(X_, Y_) X_ ## Y_

// This emulation can be used multiple times on one line (and thus in
// a macro), except at class scope
# define static_assert(b_, m_) \
  typedef int CONCAT_(sa_, __LINE__)[b_ ? 1 : -1]
#endif


//===========================================================================//
// __has_constant_initializer
//===========================================================================//
#define ATTR __attribute__((require_constant_initialization))
struct PODType {
    int value;
    int value2;
};
#if __cplusplus >= 201103L
struct LitType {
    constexpr LitType() : value(0) {}
    constexpr LitType(int x) : value(x) {}
    LitType(void*) : value(-1) {}
    int value;
};
#endif

struct NonLit {
#if __cplusplus >= 201402L
    constexpr NonLit() : value(0) {}
    constexpr NonLit(int x ) : value(x) {}
#else
    NonLit() : value(0) {}
    NonLit(int x) : value(x) {}
#endif
    NonLit(void*) : value(-1) {}
    ~NonLit() {}
    int value;
};

struct StoresNonLit {
#if __cplusplus >= 201402L
    constexpr StoresNonLit() : obj() {}
    constexpr StoresNonLit(int x) : obj(x) {}
#else
    StoresNonLit() : obj() {}
    StoresNonLit(int x) : obj(x) {}
#endif
    StoresNonLit(void* p) : obj(p) {}
    NonLit obj;
};

const bool NonLitHasConstInit =
#if __cplusplus >= 201402L
    true;
#else
    false;
#endif

// Test diagnostics when the argument does not refer to a named identifier
void check_is_constant_init_bogus()
{
    (void)__has_constant_initializer(42); // expected-error {{expression must be a named variable}}
    (void)__has_constant_initializer(ReturnInt()); // expected-error {{expression must be a named variable}}
    (void)__has_constant_initializer(42, 43); // expected-error {{expression must be a named variable}}
}

// [basic.start.static]p2.1
// if each full-expression (including implicit conversions) that appears in
// the initializer of a reference with static or thread storage duration is
// a constant expression (5.20) and the reference is bound to a glvalue
// designating an object with static storage duration, to a temporary object
// (see 12.2) or subobject thereof, or to a function;

// Test binding to a static glvalue
const int glvalue_int = 42;
const int glvalue_int2 = ReturnInt();
ATTR const int& glvalue_ref ATTR = glvalue_int;
ATTR const int& glvalue_ref2 ATTR = glvalue_int2;
ATTR __thread const int& glvalue_ref_tl = glvalue_int;

void test_basic_start_static_2_1() {
    const int non_global = 42;
    ATTR static const int& local_init = non_global; // expected-error {{variable does not have a constant initializer}}
    ATTR static const int& global_init = glvalue_int;
    ATTR static const int& temp_init = 42;
#if 0
    /// FIXME: Why is this failing?
    __thread const int& tl_init = 42;
    static_assert(__has_constant_initializer(tl_init), "");
#endif
}

ATTR const int& temp_ref = 42;
ATTR const int& temp_ref2 = ReturnInt(); // expected-error {{variable does not have a constant initializer}}
ATTR const NonLit& nl_temp_ref = 42; // expected-error {{variable does not have a constant initializer}}

#if __cplusplus >= 201103L
ATTR const LitType& lit_temp_ref = 42;
ATTR const int& subobj_ref = LitType{}.value;
#endif

ATTR const int& nl_subobj_ref = NonLit().value;  // expected-error {{variable does not have a constant initializer}}

struct TT1 {
  ATTR static const int& no_init; 
  ATTR static const int& glvalue_init;
  ATTR static const int& temp_init;
  ATTR static const int& subobj_init;
#if __cplusplus >= 201103L
  ATTR static thread_local const int& tl_glvalue_init;
  ATTR static thread_local const int& tl_temp_init;
#endif
};
const int& TT1::glvalue_init = glvalue_int;
const int& TT1::temp_init = 42;
const int& TT1::subobj_init = PODType().value;
#if __cplusplus >= 201103L
thread_local const int& TT1::tl_glvalue_init = glvalue_int;
thread_local const int& TT1::tl_temp_init = 42; // expected-error {{variable does not have a constant initializer}}
#endif

// [basic.start.static]p2.2
// if an object with static or thread storage duration is initialized by a
// constructor call, and if the initialization full-expression is a constant
// initializer for the object;

void test_basic_start_static_2_2()
{
    ATTR static PODType pod;
    ATTR static PODType pot2 = {ReturnInt()}; // expected-error {{variable does not have a constant initializer}}

#if __cplusplus >= 201103L
    constexpr LitType l;
    ATTR static LitType static_lit = l;
    ATTR static LitType static_lit2 = (void*)0; // expected-error {{variable does not have a constant initializer}}
    ATTR static LitType static_lit3 = ReturnInt(); // expected-error {{variable does not have a constant initializer}}
    ATTR thread_local LitType tls = 42;
#endif
}

struct TT2 {
  ATTR static PODType pod_noinit;
  ATTR static PODType pod_copy_init;
#if __cplusplus >= 201103L
  ATTR static constexpr LitType lit = {};
  ATTR static const NonLit non_lit;
  ATTR static const NonLit non_lit_copy_init;
#endif
};
PODType TT2::pod_noinit;
PODType TT2::pod_copy_init(TT2::pod_noinit); // expected-error {{variable does not have a constant initializer}}
#if __cplusplus >= 201103L
const NonLit TT2::non_lit(42);
const NonLit TT2::non_lit_copy_init = 42; // expected-error {{variable does not have a constant initializer}}
#endif

#if __cplusplus >= 201103L
LitType lit_ctor;
LitType lit_ctor2{};
LitType lit_ctor3 = {};
__thread LitType lit_ctor_tl = {};
static_assert(__has_constant_initializer(lit_ctor), "");
static_assert(__has_constant_initializer(lit_ctor2), "");
static_assert(__has_constant_initializer(lit_ctor3), "");
static_assert(__has_constant_initializer(lit_ctor_tl), "");

#if __cplusplus >= 201402L
ATTR NonLit nl_ctor;
ATTR NonLit nl_ctor2{};
ATTR NonLit nl_ctor3 = {};
ATTR thread_local NonLit nl_ctor_tl = {};
ATTR StoresNonLit snl;
#else
ATTR NonLit nl_ctor; // expected-error {{variable does not have a constant initializer}}
ATTR NonLit nl_ctor2{}; // expected-error {{variable does not have a constant initializer}}
ATTR NonLit nl_ctor3 = {}; // expected-error {{variable does not have a constant initializer}}
ATTR thread_local NonLit nl_ctor_tl = {}; // expected-error {{variable does not have a constant initializer}}
ATTR StoresNonLit snl; // expected-error {{variable does not have a constant initializer}}
#endif

// Non-literal types cannot appear in the initializer of a non-literal type.
ATTR int nl_in_init = NonLit{42}.value; // expected-error {{variable does not have a constant initializer}}
ATTR int lit_in_init = LitType{42}.value;
#endif

// [basic.start.static]p2.3
// if an object with static or thread storage duration is not initialized by a
// constructor call and if either the object is value-initialized or every
// full-expression that appears in its initializer is a constant expression.
void test_basic_start_static_2_3()
{
    ATTR static int static_local = 42;
    ATTR static int static_local2; // expected-error {{variable does not have a constant initializer}}
#if __cplusplus >= 201103L
    ATTR thread_local int tl_local = 42;
#endif
}

ATTR int no_init; // expected-error {{variable does not have a constant initializer}}
ATTR int arg_init = 42;
ATTR PODType pod_init = {};
ATTR PODType pod_missing_init = {42 /* should have second arg */};
ATTR PODType pod_full_init = {1, 2};
ATTR PODType pod_non_constexpr_init = {1, ReturnInt()}; // expected-error {{variable does not have a constant initializer}}

#if __cplusplus >= 201103L
ATTR int val_init{};
ATTR int brace_init = {};
#endif

ATTR __thread int tl_init = 0;

#if __cplusplus >= 201103L

struct NotC { constexpr NotC(void*) {} NotC(int) {} };
template <class T>
struct TestCtor {
  constexpr TestCtor(int x) : value(x) {}
  T value;
};

ATTR TestCtor<NotC> t(42); // expected-error {{variable does not have a constant initializer}}
#endif
