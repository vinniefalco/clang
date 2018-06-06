// RUN: %clang -target x86_64-apple-macosx10.12 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target arm64-apple-ios10 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target arm64-apple-tvos10 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target thumbv7-apple-watchos3 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mios-simulator-version-min=10 \
// RUN:  -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mtvos-simulator-version-min=10 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mwatchos-simulator-version-min=3 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=UNAVAILABLE
//
// Test that only GCC versions under 7.0 say aligned allocation is unavailable.
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     --target=i386-unknown-linux -m32 \
// RUN:     -ccc-install-dir %S/Inputs/gcc_version_parsing5/bin \
// RUN:     --gcc-toolchain="" \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
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
// RUN:     --gcc-toolchain="" \
// RUN: | FileCheck -check-prefix=UNAVAILABLE %s
//
// Test that libc++ versions with __libcpp_version < 4000 consider it unavailable
//
// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1 \
// RUN:     -target x86_64-unknown-linux-gnu \
// RUN:     -stdlib=libc++ \
// RUN:     -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:     --sysroot=%S/Inputs/basic_linux_libcxx_tree_libcpp_ver_3000/ \
// RUN:     --gcc-toolchain=""  \
// RUN: | FileCheck -check-prefix=UNAVAILABLE %s

// UNAVAILABLE: "-faligned-allocation-unavailable"

// RUN: %clang -target x86_64-apple-macosx10.13 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target arm64-apple-ios11 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target arm64-apple-tvos11 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target armv7k-apple-watchos4 -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mios-simulator-version-min=11 \
// RUN:  -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mtvos-simulator-version-min=11 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-darwin -mwatchos-simulator-version-min=4 \
// RUN: -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// Check that passing -faligned-allocation, -fno-aligned-allocation,
// -stdlib=libstdc++, or -nostdinc++ stops the Darwin driver from passing
// -faligned-allocation-unavailable to cc1.
//
// RUN: %clang -target x86_64-apple-macosx10.12 -faligned-allocation -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-macosx10.12 -fno-aligned-allocation -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-macosx10.12 -nostdinc++ -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -target x86_64-apple-macosx10.12 -stdlib=libstdc++ -c -### %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=AVAILABLE
//
// Test that GCC versions >= 7.0 consider it available
//
// RUN: %clang -no-canonical-prefixes %s -### -c 2>&1 \
// RUN:     --target=i386-unknown-linux -m32 \
// RUN:     -ccc-install-dir %S/Inputs/gcc_version_parsing7/bin \
// RUN:     --gcc-toolchain="" \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=AVAILABLE %s
//
// Test that libc++ versions with __libcpp_version >= 4000 consider it available
//
// RUN: %clang -no-canonical-prefixes %s -### -c 2>&1 \
// RUN:     -target x86_64-unknown-linux-gnu \
// RUN:     -stdlib=libc++ \
// RUN:     -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:     --sysroot=%S/Inputs/basic_linux_libcxx_tree_libcpp_ver_4000/ \
// RUN:     --gcc-toolchain="" \
// RUN: | FileCheck -check-prefix=AVAILABLE %s
//
// Test that non-default libcxx paths are searched for __libcpp_version
//
// RUN: %clang -no-canonical-prefixes %s -### -c 2>&1 \
// RUN:   -target x86_64-unknown-linux-gnu  \
// RUN:   -stdlib=libc++ -nostdinc++ \
// RUN:    -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:    --sysroot=%S/Inputs/basic_linux_libcxx_tree/ \
// RUN:    -cxx-isystem %S/Inputs/basic_linux_libcxx_tree_libcpp_ver_4000/usr/include/c++/v1 \
// RUN:    --gcc-toolchain="" \
// RUN:    | FileCheck %s -check-prefix=AVAILABLE
//
// RUN: %clang -no-canonical-prefixes %s -### -c 2>&1 \
// RUN:   -target x86_64-unknown-linux-gnu  \
// RUN:   -stdlib=libc++ -nostdinc++ \
// RUN:    -ccc-install-dir %S/Inputs/basic_linux_tree/usr/bin \
// RUN:    --sysroot=%S/Inputs/basic_linux_libcxx_tree/ \
// RUN:    -I %S/Inputs/basic_linux_libcxx_tree_libcpp_ver_4000/usr/include/c++/v1 \
// RUN:    --gcc-toolchain="" \
// RUN:    | FileCheck %s -check-prefix=AVAILABLE

// AVAILABLE-NOT: "-faligned-allocation-unavailable"
