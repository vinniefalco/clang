// RUN: %clang -target x86_64-apple-macosx10.11 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target arm64-apple-ios9 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target arm64-apple-tvos9 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target thumbv7-apple-watchos2 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mios-simulator-version-min=9 \
// RUN:  -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mtvos-simulator-version-min=9 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mwatchos-simulator-version-min=2 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// UNAVAILABLE: "-fsized-deallocation-unavailable"

// RUN: %clang -target x86_64-apple-macosx10.12 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target arm64-apple-ios10 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target arm64-apple-tvos10 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target armv7k-apple-watchos3 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-unknown-linux-gnu -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mios-simulator-version-min=10 \
// RUN:  -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mtvos-simulator-version-min=10 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mwatchos-simulator-version-min=3 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// Check that passing -fsized-deallocation, -fno-sized-deallocation, or
// -nostdinc++ stops the driver from passing -fsized-deallocation-unavailable to cc1.
//
// RUN: %clang -target x86_64-apple-macosx10.11 -fsized-deallocation -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-macosx10.11 -fno-sized-deallocation -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-macosx10.11 -nostdinc++ -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE

// AVAILABLE-NOT: "-fsized-deallocation-unavailable"
