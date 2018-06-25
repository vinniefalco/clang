
struct InMemInit {

  int info = __builtin_LINE();
  InMemInit() = default;
  constexpr InMemInit(int) {}
};
//static_assert(InMemInit{}.check(__LINE__ - 3), "");
//static_assert(InMemInit{42}.check(__LINE__ - 3), "");
InMemInit i = {};
