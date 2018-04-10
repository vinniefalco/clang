#include "Inputs/std-compare.h"

struct Foo {
  void foo() {}
};

using MPtr = void (Foo::*)();

void bar() {
  MPtr x =
}
