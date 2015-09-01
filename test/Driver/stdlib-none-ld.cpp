// RUN: %clangxx  -target x86_64-unknown-linux-gnu -### -stdlib=none %s 2> %t
// RUN: FileCheck < %t %s
// RUN: %clangxx -target  x86_64-unknown-linux-gnu -### -stdlib=libc++ %s 2> %t
// RUN: FileCheck --check-prefix=CHECK-SANITY < %t %s
//
// CHECK-NOT: "-lc++"
// CHECK-NOT: "-lstdc++"
// CHECK: "-lgcc"
// CHECK: "-lc"
// CHECK: crtend
// CHECK: crtn

// CHECK-SANITY: "-lc++"