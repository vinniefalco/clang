// RUN: %clang_cc1 -triple=x86_64-linux-gnu -emit-llvm -fstrict-vtable-pointers -o - %s \
// RUN: | FileCheck --check-prefixes=CHECK,CHECK-STRICT %s
// RUN: %clang_cc1 -triple=x86_64-linux-gnu -emit-llvm -o - %s \
// RUN: | FileCheck --check-prefixes=CHECK,CHECK-NONSTRICT %s

//===----------------------------------------------------------------------===//
//                            Positive Cases
//===----------------------------------------------------------------------===//

struct TestVirtualFn {
  virtual void foo() {}
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_fn
extern "C" void test_builtin_launder_virtual_fn(TestVirtualFn *p) {
  // CHECK: entry
  // CHECK-NEXT: %p.addr = alloca [[TYPE:%.*]], align 8
  // CHECK-NEXT: %d = alloca [[TYPE]]
  // CHECK-NEXT: store [[TYPE]] %p, [[TYPE]]* %p.addr
  // CHECK-NEXT: [[TMP0:%.*]] = load [[TYPE]], [[TYPE]]* %p.addr

  // CHECK-NONSTRICT-NEXT: store [[TYPE]] [[TMP0]], [[TYPE]]* %d

  // CHECK-STRICT-NEXT: [[TMP1:%.*]] = bitcast [[TYPE]] [[TMP0]] to i8*
  // CHECK-STRICT-NEXT: [[TMP2:%.*]] = call i8* @llvm.invariant.group.barrier.p0i8(i8* [[TMP1]])
  // CHECK-STRICT-NEXT: [[TMP3:%.*]] = bitcast i8* [[TMP2]] to [[TYPE]]
  // CHECK-STRICT-NEXT: store [[TYPE]] [[TMP3]], [[TYPE]]* %d

  // CHECK-NEXT: ret void
  TestVirtualFn *d = __builtin_launder(p);
}

struct TestPolyBase : TestVirtualFn {
};

// CHECK-LABEL: define void @test_builtin_launder_poly_base
extern "C" void test_builtin_launder_poly_base(TestPolyBase *p) {
  // CHECK-STRICT-NOT: ret void
  // CHECK-STRICT: @llvm.invariant.group.barrier

  // CHECK-NONSTRICT-NOT: @llvm.invariant.group.barrier

  // CHECK: ret void
  TestPolyBase *d = __builtin_launder(p);
}

struct TestBase {};
struct TestVirtualBase : virtual TestBase {};

// CHECK-LABEL: define void @test_builtin_launder_virtual_base
extern "C" void test_builtin_launder_virtual_base(TestVirtualBase *p) {
  // CHECK-STRICT-NOT: ret void
  // CHECK-STRICT: @llvm.invariant.group.barrier

  // CHECK-NONSTRICT-NOT: @llvm.invariant.group.barrier

  // CHECK: ret void
  TestVirtualBase *d = __builtin_launder(p);
}

//===----------------------------------------------------------------------===//
//                            Negative Cases
//===----------------------------------------------------------------------===//

// CHECK-LABEL: define void @test_builtin_launder_ommitted_one
extern "C" void test_builtin_launder_ommitted_one(int *p) {
  // CHECK: entry
  // CHECK-NEXT: %p.addr = alloca i32*
  // CHECK-NEXT: %d = alloca i32*
  // CHECK-NEXT: store i32* %p, i32** %p.addr, align 8
  // CHECK-NEXT: [[TMP:%.*]] = load i32*, i32** %p.addr
  // CHECK-NEXT: store i32* [[TMP]], i32** %d
  // CHECK-NEXT: ret void
  int *d = __builtin_launder(p);
}

struct TestNoInvariant {
  int x;
};

// CHECK-LABEL: define void @test_builtin_launder_ommitted_two
extern "C" void test_builtin_launder_ommitted_two(TestNoInvariant *p) {
  // CHECK: entry
  // CHECK-NEXT: %p.addr = alloca [[TYPE:%.*]], align 8
  // CHECK-NEXT: %d = alloca [[TYPE]]
  // CHECK-NEXT: store [[TYPE]] %p, [[TYPE]]* %p.addr
  // CHECK-NEXT: [[TMP:%.*]] = load [[TYPE]], [[TYPE]]* %p.addr
  // CHECK-NEXT: store [[TYPE]] [[TMP]], [[TYPE]]* %d
  // CHECK-NEXT: ret void
  TestNoInvariant *d = __builtin_launder(p);
}

struct TestVirtualMember {
  TestVirtualFn member;
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_member
extern "C" void test_builtin_launder_virtual_member(TestVirtualMember *p) {
  // CHECK: entry
  // CHECK-NONSTRICT-NOT: @llvm.invariant.group.barrier
  // CHECK-STRICT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestVirtualMember *d = __builtin_launder(p);
}

struct TestVirtualMemberDepth2 {
  TestVirtualMember member;
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_member_depth_2
extern "C" void test_builtin_launder_virtual_member_depth_2(TestVirtualMemberDepth2 *p) {
  // CHECK: entry
  // CHECK-NONSTRICT-NOT: @llvm.invariant.group.barrier
  // CHECK-STRICT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestVirtualMemberDepth2 *d = __builtin_launder(p);
}

struct TestVirtualReferenceMember {
  TestVirtualFn &member;
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_reference_member
extern "C" void test_builtin_launder_virtual_reference_member(TestVirtualReferenceMember *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestVirtualReferenceMember *d = __builtin_launder(p);
}

struct TestRecursiveMember {
  TestRecursiveMember &member;
};

// CHECK-LABEL: define void @test_builtin_launder_recursive_member
extern "C" void test_builtin_launder_recursive_member(TestRecursiveMember *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestRecursiveMember *d = __builtin_launder(p);
}

struct TestVirtualRecursiveMember {
  TestVirtualRecursiveMember &member;
  virtual void foo();
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_recursive_member
extern "C" void test_builtin_launder_virtual_recursive_member(TestVirtualRecursiveMember *p) {
  // CHECK: entry
  // CHECK-NONSTRICT-NOT: @llvm.invariant.group.barrier
  // CHECK-STRICT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestVirtualRecursiveMember *d = __builtin_launder(p);
}

/// The test cases in this namespace technically need to be laundered according
/// to the language in the standard (ie they have const or reference subobjects)
/// but LLVM doesn't currently optimize on these cases -- so Clang emits
/// __builtin_launder as a nop.
///
/// NOTE: Adding optimizations for these cases later is an LTO ABI break. That's
/// probably OK for now -- but is something to keep in mind.
namespace pessimizing_cases {

struct TestConstMember {
  const int x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_member
extern "C" void test_builtin_launder_const_member(TestConstMember *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestConstMember *d = __builtin_launder(p);
}

struct TestConstSubobject {
  TestConstMember x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_subobject
extern "C" void test_builtin_launder_const_subobject(TestConstSubobject *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestConstSubobject *d = __builtin_launder(p);
}

struct TestConstObject {
  const struct TestConstMember x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_object
extern "C" void test_builtin_launder_const_object(TestConstObject *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestConstObject *d = __builtin_launder(p);
}

struct TestReferenceMember {
  int &x;
};

// CHECK-LABEL: define void @test_builtin_launder_reference_member
extern "C" void test_builtin_launder_reference_member(TestReferenceMember *p) {
  // CHECK: entry
  // CHECK-NOT: @llvm.invariant.group.barrier
  // CHECK: ret void
  TestReferenceMember *d = __builtin_launder(p);
}

} // namespace pessimizing_cases
