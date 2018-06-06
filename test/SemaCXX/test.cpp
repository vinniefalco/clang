
namespace std {
typedef decltype(sizeof(0)) size_t;
enum class align_val_t : std::size_t {};
struct nothrow_t {};
nothrow_t nothrow;
} // namespace std

void *operator new(std::size_t __sz, const std::nothrow_t &) noexcept;
void *operator new[](std::size_t __sz, const std::nothrow_t &) noexcept;

void *operator new(std::size_t __sz, const std::nothrow_t &) noexcept;
void *operator new[](std::size_t __sz, const std::nothrow_t &) noexcept;
void operator delete(void *, std::size_t) noexcept;
void operator delete[](void *, std::size_t) noexcept;

void *operator new(std::size_t, std::size_t, long long);

struct S {
  int x[16];
};

void test() {
  void *v = operator new(42);
  operator delete(v, 42);
}
