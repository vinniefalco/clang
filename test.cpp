
struct Foo {
  int operator()(int);
  long operator()(long);
};

extern int X;

void bar() {
  Foo f;
  f(X);
}
