struct NonLit {
    constexpr NonLit() : value(0) {}
    constexpr NonLit(int x ) : value(x) {}
    NonLit(void*) : value(-1) {}
    ~NonLit() {}
    int value;
};
NonLit nlobj_default_init;
NonLit nlobj_list_init = {};
const NonLit nlobj_direct_init(42);
static_assert(__is_constant_initialized(nlobj_default_init), "");
static_assert(__is_constant_initialized(nlobj_list_init), "");
static_assert(__is_constant_initialized(nlobj_direct_init), "");
