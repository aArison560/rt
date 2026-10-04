// Tests de rt::Arena / rt::FixedVector (T014). Programme autonome (Catch2 en T017) :
// renvoie ≠ 0 si un contrôle échoue.

#include <cstdint>
#include <iostream>

#include "rt/base/Arena.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
	if (!(cond)) {                                                                             \
	    ++g_failures;                                                                          \
	    std::cerr << "FAIL line " << __LINE__ << ": " #cond << '\n';                           \
	}                                                                                          \
    } while (false)

} // namespace

int main() {
    using namespace rt;

    // --- Arena : alignement -------------------------------------------------------------
    Arena arena(4096);
    void* p1 = arena.alloc(8, 8);
    CHECK(p1 != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(p1) % 8 == 0);

    (void)arena.alloc(3, 1); // désaligne volontairement
    void* p2 = arena.alloc(16, 16);
    CHECK(p2 != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(p2) % 16 == 0);

    auto* f = arena.alloc<float>(4);
    CHECK(f != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(f) % alignof(float) == 0);

    // alignement non puissance de 2 → erreur propre
    CHECK(arena.alloc(8, 3) == nullptr);
    CHECK(arena.alloc(0, 8) == nullptr);

    // --- Arena : débordement → nullptr, pas de crash ------------------------------------
    Arena small(64);
    CHECK(small.alloc(64, 8) != nullptr);
    CHECK(small.alloc(1, 8) == nullptr); // plein
    CHECK(small.remaining() < 8);

    // --- Arena : reset réutilisable ------------------------------------------------------
    small.reset();
    CHECK(small.used() == 0);
    void* again = small.alloc(64, 8);
    CHECK(again != nullptr);

    Arena big(1024);
    void* first = big.alloc(128, 8);
    big.reset();
    void* second = big.alloc(128, 8);
    CHECK(first == second); // après reset, on réutilise les mêmes octets

    // --- FixedVector ---------------------------------------------------------------------
    FixedVector<int, 3> v;
    CHECK(v.empty() && !v.full());
    CHECK(v.push(1) && v.push(2) && v.push(3));
    CHECK(v.full());
    CHECK(!v.push(4)); // débordement → code d'erreur, non crash
    CHECK(v.size() == 3);
    CHECK(v[0] == 1 && v[2] == 3);
    v.clear();
    CHECK(v.empty() && v.size() == 0);
    CHECK(v.push(42) && v[0] == 42);

    if (g_failures == 0)
	std::cout << "test_arena: OK\n";
    return g_failures == 0 ? 0 : 1;
}
