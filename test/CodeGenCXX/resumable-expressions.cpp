// RUN: %clang_cc1 -triple x86_64-apple-darwin10.0.0 -fresumable-functions -emit-llvm -o - %s -fexceptions -std=c++17 | FileCheck %s

resumable int foo() {
  return 42;
}

void bar() {
  resumable auto r = foo();
  bool b = r.ready();
  int x = r.result();
}

struct NonTrivial {
  NonTrivial();
  NonTrivial(NonTrivial const &);
  ~NonTrivial();
};
resumable NonTrivial test_dtor() {
  return NonTrivial{};
}
void test() {
  resumable auto r = test_dtor();
}
