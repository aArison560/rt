// Tests de rt::Arena / rt::FixedVector (T014), Catch2 (T017).

#include <catch2/catch_amalgamated.hpp>

#include <cstdint>

#include "rt/base/Arena.hpp"

TEST_CASE("Arena : alignement des allocations", "[arena]") {
    using namespace rt;

    Arena arena(4096);
    void* p1 = arena.alloc(8, 8);
    REQUIRE(p1 != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(p1) % 8 == 0);

    (void)arena.alloc(3, 1); // désaligne volontairement
    void* p2 = arena.alloc(16, 16);
    REQUIRE(p2 != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(p2) % 16 == 0);

    auto* f = arena.alloc<float>(4);
    REQUIRE(f != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(f) % alignof(float) == 0);
}

TEST_CASE("Arena : alignement invalide → nullptr, jamais de throw", "[arena]") {
    using namespace rt;

    Arena arena(4096);
    REQUIRE(arena.alloc(8, 3) == nullptr);
    REQUIRE(arena.alloc(0, 8) == nullptr);
}

TEST_CASE("Arena : débordement → nullptr, pas de crash", "[arena]") {
    using namespace rt;

    Arena small(64);
    REQUIRE(small.alloc(64, 8) != nullptr);
    REQUIRE(small.alloc(1, 8) == nullptr); // plein
    REQUIRE(small.remaining() < 8);
}

TEST_CASE("Arena : reset réutilise les mêmes octets", "[arena]") {
    using namespace rt;

    Arena small(64);
    small.alloc(64, 8);
    small.reset();
    REQUIRE(small.used() == 0);
    REQUIRE(small.alloc(64, 8) != nullptr);

    Arena big(1024);
    void* first = big.alloc(128, 8);
    big.reset();
    void* second = big.alloc(128, 8);
    REQUIRE(first == second); // après reset, on réutilise les mêmes octets
}

TEST_CASE("FixedVector : capacité fixe, débordement → erreur propre", "[arena]") {
    using namespace rt;

    FixedVector<int, 3> v;
    REQUIRE(v.empty());
    REQUIRE_FALSE(v.full());
    REQUIRE(v.push(1));
    REQUIRE(v.push(2));
    REQUIRE(v.push(3));
    REQUIRE(v.full());
    REQUIRE_FALSE(v.push(4)); // débordement → code d'erreur, non crash
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[2] == 3);
    v.clear();
    REQUIRE(v.empty());
    REQUIRE(v.size() == 0);
    REQUIRE(v.push(42));
    REQUIRE(v[0] == 42);
}
