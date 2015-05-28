// RUN: %clang_cc1 -verify %s -DTEST_ALL
// RUN: %clang_cc1 -verify=note %s -DTEST_ALL
// RUN: %clang_cc1 -verify=error %s -DTEST_ALL
#ifdef TEST_ALL
int x; // expected-note {{previous definition is here}}
float x; // expected-error {{redefinition of 'x'}}
#warning AAA
    // expected-warning@-1 {{AAA}}
#endif

// RUN: %clang_cc1 -verify=remark %s -DTEST_WARN
// RUN: %clang_cc1 -verify=warning %s -DTEST_WARN
#ifdef TEST_WARN
int x;
float x; // expected-error {{redefinition of 'x'}}
#warning AAA
    // expected-warning@-1 {{AAA}}
#endif

// RUN: %clang_cc1 -verify=error %s -DTEST_ERR
// RUN: not %clang_cc1 -DTEST_ERR -verify=warning %s 2>&1 | FileCheck -check-prefix=CHECK-WARN %s
// RUN: not %clang_cc1 -DTEST_ERR -verify=note %s 2>&1 | FileCheck -check-prefix=CHECK-NOTE %s
#ifdef TEST_ERR
int x;
float x; // expected-error {{redefinition of 'x'}}
#warning AAA

// CHECK-WARN: error: 'warning' diagnostics seen but not expected
// CHECK-WARN-NEXT: Line 26: AAA
// CHECK-WARN-NEXT: 1 error generated

// CHECK-NOTE: error: 'warning' diagnostics seen but not expected
// CHECK-NOTE-NEXT: Line 26: AAA
// CHECK-NOTE-NEXT: error: 'note' diagnostics seen but not expected:
// CHECK-NOTE-NEXT: Line 24: previous definition is here
// CHECK-NOTE-NEXT: 2 errors generated
#endif

