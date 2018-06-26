
template <int>
struct Printer;
constexpr bool test() {
  constexpr auto *n = __builtin_FUNCTION();
  constexpr char c = n[0];
  constexpr int x = c == 't';
  return x;
}
static_assert(test());
