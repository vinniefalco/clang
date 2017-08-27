struct SL {
  static constexpr SL current(
          const char* f = __builtin_FUNCTION(),
          unsigned l = __builtin_LINE()) {
    return {f, l};
  }
  const char* function;
  unsigned line;
};

constexpr bool is_equal(const char *LHS, const char *RHS) {
  while (*LHS != 0 && *RHS != 0) {
    if (*LHS != *RHS)
      return false;
    ++LHS;
    ++RHS;
  }
  return *LHS == 0 && *RHS == 0;
}


template <class T, class U = SL>
constexpr const char* test_func_template(T, U u = U::current()) {
  static_assert(is_equal(U::current().function, __func__));
  static_assert(is_equal(U::current().function, ""));
  static_assert(U::current().line==  __LINE__);
  static_assert(U::current().line==  __LINE__);

  return u.function;
}
template <class T>
void func_template_tests() {
  constexpr auto P = test_func_template(42);
}
template void func_template_tests<int>();
