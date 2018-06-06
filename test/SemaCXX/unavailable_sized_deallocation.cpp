// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -fexceptions -fsized-deallocation-unavailable -std=c++14 -verify %s
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -fexceptions -std=c++14 -verify -DNO_ERRORS %s
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -fexceptions -fsized-deallocation -fsized-deallocation-unavailable -std=c++14 -verify %s
// RUN: %clang_cc1 -triple arm64-apple-ios9.0.0 -fexceptions -fsized-deallocation-unavailable -std=c++14 -verify -DIOS %s
// RUN: %clang_cc1 -triple arm64-apple-ios9.0.0 -fexceptions -std=c++14 -verify -DNO_ERRORS %s
// RUN: %clang_cc1 -triple arm64-apple-tvos9.0.0 -fexceptions -fsized-deallocation-unavailable -std=c++14 -verify -DTVOS %s
// RUN: %clang_cc1 -triple arm64-apple-tvos9.0.0 -fexceptions -std=c++14 -verify -DNO_ERRORS %s
// RUN: %clang_cc1 -triple armv7k-apple-watchos2.0.0 -fexceptions -fsized-deallocation-unavailable -std=c++14 -verify -DWATCHOS %s
// RUN: %clang_cc1 -triple armv7k-apple-watchos2.0.0 -fexceptions -std=c++14 -verify -DNO_ERRORS %s

#ifdef NO_ERRORS
// expected-no-diagnostics
#endif

namespace std {
typedef decltype(sizeof(0)) size_t;
enum class align_val_t : std::size_t {};
struct nothrow_t {};
nothrow_t nothrow;
} // namespace std

void *operator new(std::size_t __sz, const std::nothrow_t &) noexcept;
void *operator new[](std::size_t __sz, const std::nothrow_t &) noexcept;

void *operator new(std::size_t __sz, const std::nothrow_t &) noexcept;
void *operator new[](std::size_t __sz, const std::nothrow_t &) noexcept;
void operator delete(void *, std::size_t) noexcept;
void operator delete[](void *, std::size_t) noexcept;

void *operator new(std::size_t, std::size_t, long long);

struct S {
  int x[16];
};

void test() {
  auto *p = new S;
#ifndef NO_ERRORS
// expected-error@-2 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on}}
// expected-note@-3 {{if you supply your own sized deallocation functions}}
#endif

  delete p;
#ifndef NO_ERRORS
// expected-error@-2 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on}}
// expected-note@-3 {{if you supply your own sized deallocation functions}}
#endif

  auto *pa = new S[4];
#ifndef NO_ERRORS
// expected-error@-2 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on}}
// expected-note@-3 {{if you supply your own sized deallocation functions}}
#endif

  delete[] pa; // OK, doesn't call sized delete.

  void *v = operator new(42);
  operator delete(v, 42);
#ifndef NO_ERRORS
// expected-error@-2 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on}}
// expected-note@-3 {{if you supply your own sized deallocation functions}}
#endif
}

void testCheckOS(S *p) {
  delete p;
}
#if defined(IOS)
// expected-error@-3 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on iOS 10 or newer}}}
#elif defined(TVOS)
// expected-error@-5 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on tvOS 10 or newer}}}
#elif defined(WATCHOS)
// expected-error@-7 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on watchOS 3 or newer}}}
#elif !defined(NO_ERRORS)
// expected-error@-9 {{sized deallocation function of type 'void (void *, std::size_t) noexcept' is only available on macOS 10.12 or newer}}}
#endif
#ifndef NO_ERRORS
// expected-note@-12 1 {{if you supply your own sized deallocation functions}}
#endif

// No errors if user-defined sized deallocation functions are available.
void *operator new(std::size_t __sz, std::size_t) {
  static char array[256];
  return &array;
}

void operator delete(void *p, std::size_t) {
}

void testOveraligned2() {
  auto p = new S;
  delete p;
}
