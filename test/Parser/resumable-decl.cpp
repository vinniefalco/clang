// RUN: %clang_cc1 -verify -fsyntax-only -std=c++11 -pedantic-errors \
// RUN:   -triple x86_64-linux-gnu -fresumable-functions %s

resumable int t1(); // OK

// expected-error@+1 {{'main' is not allowed to be declared resumable}}
resumable int main() {}

void t2() {
  break resumable;
}
