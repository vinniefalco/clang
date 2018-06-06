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
// Test that only GCC versions under 5.0 say aligned allocation is unavailable.
//
// RUN: %clang -target x86_64-gnu-unknown -gcc-toolchain %S/Inputs/gcc_version_parsing4/ \
// RUN:   -stdlib=libc++ -c -### %s 2>&1 \
// RUN:  | FileCheck %s -check-prefix=UNAVAILABLE
//
// Test that GCC versions under 5.0 say is unavailable.
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     --target=i386-unknown-linux -m32 \
// RUN:     -ccc-install-dir %S/Inputs/gcc_version_parsing4/bin \
// RUN:     --gcc-toolchain="" \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:     -std=c++14 \
// RUN:   | FileCheck --check-prefix=UNAVAILABLE %s
//
// Test that Libc++ versions without a __libcpp_version header consider it
// unavailable
//
// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1 \
// RUN:     -target x86_64-unknown-linux-gnu \
// RUN:     -stdlib=libc++ \
// RUN:     -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:     --sysroot=%S/Inputs/basic_linux_libcxx_tree/ \
// RUN:     --gcc-toolchain="" -std=c++14 \
// RUN: | FileCheck -check-prefix=UNAVAILABLE %s
//
// Test that libc++ versions with __libcpp_version < 4000 consider it unavailable
//
// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1 \
// RUN:     -target x86_64-unknown-linux-gnu \
// RUN:     -stdlib=libc++ \
// RUN:     -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:     --sysroot=%S/Inputs/basic_linux_libcxx_tree_libcpp_ver_3000/ \
// RUN:     --gcc-toolchain="" -std=c++14 \
// RUN: | FileCheck -check-prefix=UNAVAILABLE %s

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
//
// Test that GCC versions >= 5.0 consider it available
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     --target=i386-unknown-linux -m32 \
// RUN:     -ccc-install-dir %S/Inputs/gcc_version_parsing5/bin \
// RUN:     --gcc-toolchain="" \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=AVAILABLE %s
//
// Test that libc++ versions with __libcpp_version >= 4000 consider it available
//
// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1 \
// RUN:     -target x86_64-unknown-linux-gnu \
// RUN:     -stdlib=libc++ \
// RUN:     -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:     --sysroot=%S/Inputs/basic_linux_libcxx_tree_libcpp_ver_4000/ \
// RUN:     --gcc-toolchain="" -std=c++14 \
// RUN: | FileCheck -check-prefix=AVAILABLE %s

// AVAILABLE-NOT: "-fsized-deallocation-unavailable"
