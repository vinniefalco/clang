// RUN: %clang_cc1 -emit-llvm -o %t %s
// RUN: not grep __builtin %t
// RUN: %clang_cc1 %s -emit-llvm -o - -triple x86_64-darwin-apple | FileCheck %s

// CHECK-LABEL: define void @test_builtin_launder_ommitted_one
void test_builtin_launder_ommitted_one(int *p) {
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
void test_builtin_launder_ommitted_two(struct TestNoInvariant *p) {
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
void test_builtin_launder_const_member(struct TestConstMember *p) {
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
void test_builtin_launder_const_subobject(struct TestConstSubobject *p) {
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
void test_builtin_launder_const_object(struct TestConstObject *p) {
  // CHECK: entry
  // CHECK-NOT: ret void
  // CHECK: @llvm.invariant.group.barrier
  // CHECK: ret void
  struct TestConstObject *d = __builtin_launder(p);
}
