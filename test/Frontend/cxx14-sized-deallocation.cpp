

// Test libc++ versions
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++14 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v4 -verify -fsyntax-only -DEXPECT_ENABLED
//
// Test that sized deallocation is not enabled when no __libcpp_version header is found,
// or when the version is less than 4.0
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++14 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v3 -verify -fsyntax-only
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++14 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v0 -verify -fsyntax-only

// Test libstdc++ versions
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++14 \
// RUN:  -target-gcc-version 5.0.0 -verify -fsyntax-only -DEXPECT_ENABLED
//
// Test that feature is disabled for libstdc++ versions less than 5.0, or when
// no -target-gcc-version is specified by the driver.
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++14 \
// RUN:  -target-gcc-version 4.99.99 -verify -fsyntax-only
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++14 \
// RUN:  -verify -fsyntax-only

// Test MacOS versions. On Darwin platforms the OS version is used to determine
// if the feature should be enabled (and not the libc++ versions).
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.12.0 %s -fexceptions -std=c++14 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 %s -fexceptions -std=c++14 \
// RUN:    -verify
// RUN: %clang_cc1 -triple x86_64-apple-ios10.0.0 %s -fexceptions -std=c++14 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-ios9.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify
// RUN: %clang_cc1 -triple x86_64-apple-tvos10.0.0 %s -fexceptions -std=c++14 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-tvos9.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify
// RUN: %clang_cc1 -triple armv7k-apple-watchos3.0.0 %s -fexceptions -std=c++14 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple armv7k-apple-watchos2.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify

// Test that all of these deductions are not performed when sized deallocation
// has been manually enabled despite being unavailable.
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++14 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v3 -verify -fsyntax-only \
// RUN:    -fsized-deallocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++14 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v0 -verify -fsyntax-only \
// RUN:    -fsized-deallocation -DEXPECT_ENABLED
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++14 \
// RUN:  -target-gcc-version 4.99.99 -verify -fsyntax-only \
// RUN:  -fsized-deallocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++14 \
// RUN:  -verify -fsyntax-only -fsized-deallocation -DEXPECT_ENABLED
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 %s -fexceptions -std=c++14 \
// RUN:    -verify -fsized-deallocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-ios9.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify -fsized-deallocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-tvos9.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify -fsized-deallocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple armv7k-apple-watchos2.99.99 %s -fexceptions -std=c++14 \
// RUN:    -verify -fsized-deallocation -DEXPECT_ENABLED

#ifndef __cpp_sized_deallocation
#error Sized deallocation is not enabled
#endif
#ifndef EXPECT_ENABLED
// expected-error@-3 {{Sized deallocation is not enabled}}
#else
// expected-no-diagnostics
#endif
