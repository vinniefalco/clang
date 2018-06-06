// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -fexceptions -std=c++14 -verify %s \
// RUN:   -DEXPECT_DEFINED
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.13.0 -fexceptions -std=c++14 -verify %s \
// RUN:   -fsized-deallocation-unavailable
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.12.0 -fexceptions -std=c++14 -verify %s \
// RUN:   -fsized-deallocation -fsized-deallocation-unavailable

// Test that __cpp_sized_deallocation is not defined when CC1 is passed
// -fsized-deallocation-unavailable by the Darwin driver, even when sized
// deallocation is actually enabled.

// expected-no-diagnostics
#ifdef EXPECT_DEFINED
#ifndef __cpp_sized_deallocation
#error "__cpp_sized_deallocation" should be defined
#endif
#else
#ifdef __cpp_sized_deallocation
#error "__cpp_sized_deallocation" should not be defined
#endif
#endif
