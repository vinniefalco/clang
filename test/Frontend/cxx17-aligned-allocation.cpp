
// Test libc++ versions
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++17 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v4 -verify -fsyntax-only -DEXPECT_ENABLED
//
// Test that aligned allocation is not enabled when no __libcpp_version header is found,
// or when the version is less than 4.0
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++17 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v3 -verify -fsyntax-only
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++17 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v0 -verify -fsyntax-only

// Test libstdc++ versions
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++17 \
// RUN:  -target-gcc-version 7.0.0 -verify -fsyntax-only -DEXPECT_ENABLED
//
// Test that feature is disabled for libstdc++ versions less than 5.0, or when
// no -target-gcc-version is specified by the driver.
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++17 \
// RUN:  -target-gcc-version 6.99.99 -verify -fsyntax-only
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++17 \
// RUN:  -verify -fsyntax-only

// Test MacOS versions. On Darwin platforms the OS version is used to determine
// if the feature should be enabled (and not the libc++ versions).
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.13.0 %s -fexceptions -std=c++17 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.12.0 %s -fexceptions -std=c++17 \
// RUN:    -verify
// RUN: %clang_cc1 -triple x86_64-apple-ios11.0.0 %s -fexceptions -std=c++17 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-ios10.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify
// RUN: %clang_cc1 -triple x86_64-apple-tvos11.0.0 %s -fexceptions -std=c++17 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-tvos10.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify
// RUN: %clang_cc1 -triple armv7k-apple-watchos4.0.0 %s -fexceptions -std=c++17 \
// RUN:    -verify -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple armv7k-apple-watchos3.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify

// Test that all of these deductions are not performed when aligned allocation
// has been manually enabled despite being unavailable.
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++17 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v3 -verify -fsyntax-only \
// RUN:    -faligned-allocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libc++ -std=c++17 \
// RUN:    -cxx-isystem %S/Inputs/libcxx-headers/v0 -verify -fsyntax-only \
// RUN:    -faligned-allocation -DEXPECT_ENABLED
//
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++17 \
// RUN:  -target-gcc-version 4.99.99 -verify -fsyntax-only \
// RUN:  -faligned-allocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fexceptions -stdlib=libstdc++ -std=c++17 \
// RUN:  -verify -fsyntax-only -faligned-allocation -DEXPECT_ENABLED
//
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 %s -fexceptions -std=c++17 \
// RUN:    -verify -faligned-allocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-ios9.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify -faligned-allocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple x86_64-apple-tvos9.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify -faligned-allocation -DEXPECT_ENABLED
// RUN: %clang_cc1 -triple armv7k-apple-watchos2.99.99 %s -fexceptions -std=c++17 \
// RUN:    -verify -faligned-allocation -DEXPECT_ENABLED

#ifndef __cpp_aligned_new
#error aligned allocation is not enabled
#endif
#ifndef EXPECT_ENABLED
// expected-error@-3 {{aligned allocation is not enabled}}
#else
// expected-no-diagnostics
#endif
