// RUN: not %clangxx -DINCLUDE_TEST -stdlib=none %s 2>&1 | \
// RUN:     FileCheck %s --check-prefix=CHECK-INCLUDE
// RUN: %clangxx -DINCLUDE_TEST -fsyntax-only %s
// CHECK-INCLUDE: file not found
#if defined(INCLUDE_TEST)
#include <vector>
#endif

// RUN: %clangxx -stdlib=none -### %s 2>&1 | FileCheck %s --check-prefix=CHECK-INCLUDE-PATHS
// CHECK-INCLUDE-PATHS-NOT: "/include/c++/"

// MSVC has C++ headers in same directory as C headers.
// REQUIRES: non-ms-sdk
// REQUIRES: non-ps4-sdk