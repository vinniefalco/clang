// RUN: %clang_cc1 -fsyntax-only -fcontracts-ts -verify -std=c++1z %s

void foo(int a)[[gnu::unused]];
