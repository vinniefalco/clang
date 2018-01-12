// RUN: %clang_cc1 -triple=x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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
extern "C" void test_builtin_launder_ommitted_two(struct TestNoInvariant *p) {
  // CHECK: entry
  // CHECK-NEXT: %p.addr = alloca [[TYPE:%.*]], align 8
  // CHECK-NEXT: %d = alloca [[TYPE]]
  // CHECK-NEXT: store [[TYPE]] %p, [[TYPE]]* %p.addr
  // CHECK-NEXT: [[TMP:%.*]] = load [[TYPE]], [[TYPE]]* %p.addr
  // CHECK-NEXT: store [[TYPE]] [[TMP]], [[TYPE]]* %d
  // CHECK-NEXT: ret void
  struct TestNoInvariant *d = __builtin_launder(p);
}

struct TestConstMember {
  const int x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_member
extern "C" void test_builtin_launder_const_member(struct TestConstMember *p) {
  // CHECK: entry
  // CHECK-NEXT: %p.addr = alloca [[TYPE:%.*]], align 8
  // CHECK-NEXT: %d = alloca [[TYPE]]
  // CHECK-NEXT: store [[TYPE]] %p, [[TYPE]]* %p.addr
  // CHECK-NEXT: [[TMP0:%.*]] = load [[TYPE]], [[TYPE]]* %p.addr
  // CHECK-NEXT: [[TMP1:%.*]] = bitcast [[TYPE]] [[TMP0]] to i8*
  // CHECK-NEXT: [[TMP2:%.*]] = call i8* @llvm.invariant.group.barrier.p0i8(i8* [[TMP1]])
  // CHECK-NEXT: [[TMP3:%.*]] = bitcast i8* [[TMP2]] to [[TYPE]]
  // CHECK-NEXT: store [[TYPE]] [[TMP3]], [[TYPE]]* %d
  // CHECK-NEXT: ret void
  struct TestConstMember *d = __builtin_launder(p);
}

struct TestConstSubobject {
  struct TestConstMember x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_subobject
extern "C" void test_builtin_launder_const_subobject(struct TestConstSubobject *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestConstSubobject *d = __builtin_launder(p);
}

struct TestConstObject {
  const struct TestConstMember x;
};

// CHECK-LABEL: define void @test_builtin_launder_const_object
extern "C" void test_builtin_launder_const_object(struct TestConstObject *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestConstObject *d = __builtin_launder(p);
}

struct TestReferenceMember {
  int &x;
};

// CHECK-LABEL: define void @test_builtin_launder_reference_member
extern "C" void test_builtin_launder_reference_member(struct TestReferenceMember *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestReferenceMember *d = __builtin_launder(p);
}

struct TestVirtualFn {
  virtual void foo() {}
};

// CHECK-LABEL: define void @test_builtin_launder_virtual_fn
extern "C" void test_builtin_launder_virtual_fn(struct TestVirtualFn *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestVirtualFn *d = __builtin_launder(p);
}

struct TestPolyBase : TestVirtualFn {
};

// CHECK-LABEL: define void @test_builtin_launder_poly_base
extern "C" void test_builtin_launder_poly_base(struct TestPolyBase *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestPolyBase *d = __builtin_launder(p);
}

struct DummyBase {};
struct TestVirtualBase : virtual DummyBase {};

// CHECK-LABEL: define void @test_builtin_launder_virtual_base
extern "C" void test_builtin_launder_virtual_base(TestVirtualBase *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestVirtualBase *d = __builtin_launder(p);
}
