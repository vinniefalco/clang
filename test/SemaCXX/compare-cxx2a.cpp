// Force x86-64 because some of our heuristics are actually based
// on integer sizes.

// RUN: %clang_cc1 -triple x86_64-apple-darwin -fcxx-exceptions -fsyntax-only -pedantic -verify -Wsign-compare -std=c++2a %s

#include "Inputs/std-compare.h"

void self_compare() {
  int a;
  int b[3], c[3];
  (void)(a <=> a); // expected-warning {{self-comparison always evaluates to 'std::strong_ordering::equal'}}
  (void)(b <=> b); // expected-warning {{self-comparison always evaluates to 'std::strong_ordering::equal'}}
  (void)(b <=> c); // expected-warning {{array comparison always evaluates to a constant}}
}

void test0(long a, unsigned long b) {
  enum EnumA {A};
  enum EnumB {B};
  enum EnumC {C = 0x10000};
         // (a,b)

  // FIXME: <=> should never produce -Wsign-compare warnings. All the possible error
  // cases involve narrowing conversions and so are ill-formed.
  (void)(a <=> (unsigned long)b); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)(a <=> (unsigned int) b);
  (void)(a <=> (unsigned short) b);
  (void)(a <=> (unsigned char) b);
  (void)((long)a <=> b);                // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)a <=> b);                 // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((short)a <=> b);               // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((signed char)a <=> b);         // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((long)a <=> (unsigned long)b); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)a <=> (unsigned int)b);   // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((short) a <=> (unsigned short) b);
  (void)((signed char) a <=> (unsigned char) b);

#if 0
  // (A,b)
  (void)(A <=> (unsigned long) b);
  (void)(A <=> (unsigned int) b);
  (void)(A <=> (unsigned short) b);
  (void)(A <=> (unsigned char) b);
  (void)((long) A <=> b);
  (void)((int) A <=> b);
  (void)((short) A <=> b);
  (void)((signed char) A <=> b);
  (void)((long) A <=> (unsigned long) b);
  (void)((int) A <=> (unsigned int) b);
  (void)((short) A <=> (unsigned short) b);
  (void)((signed char) A <=> (unsigned char) b);

  // (a,B)
  (void)(a <=> (unsigned long) B);
  (void)(a <=> (unsigned int) B);
  (void)(a <=> (unsigned short) B);
  (void)(a <=> (unsigned char) B);
  (void)((long) a <=> B);
  (void)((int) a <=> B);
  (void)((short) a <=> B);
  (void)((signed char) a <=> B);
  (void)((long) a <=> (unsigned long) B);
  (void)((int) a <=> (unsigned int) B);
  (void)((short) a <=> (unsigned short) B);
  (void)((signed char) a <=> (unsigned char) B);

  // (C,b)
  (void)(C <=> (unsigned long) b);
  (void)(C <=> (unsigned int) b);
  (void)(C <=> (unsigned short) b); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'unsigned short' is always 'std::strong_ordering::greater'}}
  (void)(C <=> (unsigned char) b);  // expected-warning {{comparison of constant 'C' (65536) with expression of type 'unsigned char' is always 'std::strong_ordering::greater'}}
  (void)((long) C <=> b);
  (void)((int) C <=> b);
  (void)((short) C <=> b);
  (void)((signed char) C <=> b);
  (void)((long) C <=> (unsigned long) b);
  (void)((int) C <=> (unsigned int) b);
  (void)((short) C <=> (unsigned short) b);
  (void)((signed char) C <=> (unsigned char) b);

  // (a,C)
  (void)(a <=> (unsigned long) C);
  (void)(a <=> (unsigned int) C);
  (void)(a <=> (unsigned short) C);
  (void)(a <=> (unsigned char) C);
  (void)((long) a <=> C);
  (void)((int) a <=> C);
  (void)((short) a <=> C); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'short' is always 'std::strong_ordering::less'}}
  (void)((signed char) a <=> C); // expected-warning {{comparison of constant 'C' (65536) with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)((long) a <=> (unsigned long) C);
  (void)((int) a <=> (unsigned int) C);
  (void)((short) a <=> (unsigned short) C);
  (void)((signed char) a <=> (unsigned char) C);
#endif

  // (0x80000,b)
  (void)(0x80000 <=> (unsigned long) b);
  (void)(0x80000 <=> (unsigned int) b);
  (void)(0x80000 <=> (unsigned short) b); // expected-warning {{result of comparison of constant 524288 with expression of type 'unsigned short' is always 'std::strong_ordering::greater'}}
  (void)(0x80000 <=> (unsigned char) b); // expected-warning {{result of comparison of constant 524288 with expression of type 'unsigned char' is always 'std::strong_ordering::greater'}}
  (void)((long) 0x80000 <=> b);
  (void)((int) 0x80000 <=> b);
  (void)((short) 0x80000 <=> b);
  (void)((signed char) 0x80000 <=> b);
  (void)((long) 0x80000 <=> (unsigned long) b);
  (void)((int) 0x80000 <=> (unsigned int) b);
  (void)((short) 0x80000 <=> (unsigned short) b);
  (void)((signed char) 0x80000 <=> (unsigned char) b);

  // (a,0x80000)
  (void)(a <=> (unsigned long)0x80000); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)(a <=> (unsigned int) 0x80000);
  (void)(a <=> (unsigned short) 0x80000);
  (void)(a <=> (unsigned char) 0x80000);
  (void)((long) a <=> 0x80000);
  (void)((int) a <=> 0x80000);
  (void)((short) a <=> 0x80000); // expected-warning {{comparison of constant 524288 with expression of type 'short' is always 'std::strong_ordering::less'}}
  (void)((signed char) a <=> 0x80000); // expected-warning {{comparison of constant 524288 with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)((long)a <=> (unsigned long)0x80000); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)a <=> (unsigned int)0x80000);   // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((short) a <=> (unsigned short) 0x80000);
  (void)((signed char) a <=> (unsigned char) 0x80000);
}

void test5(bool b, bool b2) {
  (void)(b <=> b2);      // OK
  (void)(true <=> b);    // OK
  (void)(b <=> -10);     // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
  (void)(b <=> char(1)); // expected-error {{invalid operands to binary expression ('bool' and 'char')}}

  // FIXME: Should this be accepted when narrowing doesn't occur?
  (void)(b <=> 0); // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
  (void)(b <=> 1); // expected-error {{invalid operands to binary expression ('bool' and 'int')}}
}

void test6(signed char sc) {
  (void)(sc <=> 200); // expected-warning{{comparison of constant 200 with expression of type 'signed char' is always 'std::strong_ordering::less'}}
  (void)(200 <=> sc); // expected-warning{{comparison of constant 200 with expression of type 'signed char' is always 'std::strong_ordering::greater'}}
}

// Test many signedness combinations.
void test7(unsigned long other) {
  // Common unsigned, other unsigned, constant unsigned
  (void)((unsigned)other <=> (unsigned long)(0x1'ffff'ffff)); // expected-warning{{less}}
  (void)((unsigned)other <=> (unsigned long)(0xffff'ffff));
  (void)((unsigned long)other <=> (unsigned)(0x1'ffff'ffff));
  (void)((unsigned long)other <=> (unsigned)(0xffff'ffff));

  // Common unsigned, other signed, constant unsigned
  (void)((int)other <=> (unsigned long)(0xffff'ffff'ffff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)other <=> (unsigned long)(0x0000'0000'ffff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)other <=> (unsigned long)(0x0000'0000'0fff'ffff)); // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}
  (void)((int)other <=> (unsigned)(0x8000'0000));                // expected-error {{argument to 'operator<=>' cannot be narrowed}} expected-note {{argument is not a constant expression}}

  // Common unsigned, other unsigned, constant signed
  (void)((unsigned long)other <=> (int)(0xffff'ffff)); // expected-error {{argument to 'operator<=>' evaluates to -1, which cannot be narrowed to type 'unsigned long'}}

  // Common unsigned, other signed, constant signed
  // Should not be possible as the common type should also be signed.

  // Common signed, other signed, constant signed
  (void)((int)other <=> (long)(0xffff'ffff));           // expected-warning{{less}}
  (void)((int)other <=> (long)(0xffff'ffff'0000'0000)); // expected-warning{{greater}}
  (void)((int)other <=> (long)(0x0fff'ffff));
  (void)((int)other <=> (long)(0xffff'ffff'f000'0000));

  // Common signed, other signed, constant unsigned
  (void)((int)other <=> (unsigned char)(0xffff));
  (void)((int)other <=> (unsigned char)(0xff));

  // Common signed, other unsigned, constant signed
  (void)((unsigned char)other <=> (int)(0xff));
  (void)((unsigned char)other <=> (int)(0xffff));  // expected-warning{{less}}

  // Common signed, other unsigned, constant unsigned
  (void)((unsigned char)other <=> (unsigned short)(0xff));
  (void)((unsigned char)other <=> (unsigned short)(0x100)); // expected-warning{{less}}
  (void)((unsigned short)other <=> (unsigned char)(0xff));
}

// test void pointer diagnostics
void test8(void *vp, const void *cvp, int *ip) {
  (void)(vp <=> cvp); // expected-error {{three-way comparison with void pointer operand types ('void *' and 'const void *')}}
  (void)(vp <=> ip);  // expected-error {{three-way comparison with void pointer operand type ('void *' and 'int *')}}
  (void)(ip <=> cvp); // expected-error {{three-way comparison with void pointer operand type ('int *' and 'const void *')}}
}

void test9(long double ld, double d, float f, int i, long long ll) {
  (void)(f <=> ll); // OK, floating-point to integer is OK
  (void)(d <=> ld);
  (void)(i <=> f);
}

typedef int *INTPTR;
void test_typedef_bug(int *x, INTPTR y) {
  (void)(x <=> y);
}
