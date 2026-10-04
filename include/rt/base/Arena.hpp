// rt::Arena (T014) — bump allocator O(1), zéro allocation après initialisation.
//
// Invariants (style « Padding Invariants » de Webserv) :
//   I1 — Le stockage est alloué UNE fois au constructeur, jamais après.
//   I2 — alloc() ne lève jamais d'exception : overflow → nullptr.
//   I3 — L'alignement demandé est une puissance de 2 (sinon nullptr).
//   I4 — reset() invalide TOUS les pointeurs servis depuis la dernière
//        construction : les objets alloués doivent avoir une durée de vie
//        bornée par le reset (jamais de pointeur persistant inter-reset).
//   I5 — Aucune construction/destruction implicite de T : alloc() renvoie
//        de la mémoire brute, l'appelant place ses objets (placement new)
//        ou utilise FixedVector qui value-initialise.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rt {

class Arena {
  public:
    explicit Arena(std::size_t capacity) : capacity_(capacity), storage_(capacity) {}

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = default;
    Arena& operator=(Arena&&) = default;

    // O(1) : arrondi de l'offset, test de capacité, avance du compteur.
    // Jamais d'appel à malloc après le constructeur (rèle R3).
    void* alloc(std::size_t n, std::size_t alignment = alignof(std::max_align_t)) noexcept {
	if (n == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
	    return nullptr;
	const auto base = reinterpret_cast<std::uintptr_t>(storage_.data());
	const auto current = base + offset_;
	const auto aligned = (current + (alignment - 1)) & ~(alignment - 1);
	const auto padding = static_cast<std::size_t>(aligned - current);
	if (offset_ + padding > capacity_ || n > capacity_ - offset_ - padding)
	    return nullptr; // débordement → code d'erreur (nullptr), pas de crash
	offset_ += padding;
	void* ptr = storage_.data() + offset_;
	offset_ += n;
	return ptr;
    }

    template <typename T> T* alloc(std::size_t count = 1) noexcept {
	return static_cast<T*>(alloc(count * sizeof(T), alignof(T)));
    }

    void reset() noexcept { offset_ = 0; }

    [[nodiscard]] std::size_t used() const noexcept { return offset_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return capacity_ - offset_; }

  private:
    std::size_t capacity_;
    std::vector<std::byte> storage_;
    std::size_t offset_ = 0;
};

// Capacité fixe à la construction, débordement → false (code d'erreur),
// jamais d'allocation dynamique (rèle R3).
template <typename T, std::size_t N> class FixedVector {
  public:
    FixedVector() = default;

    bool push(const T& value) noexcept {
	if (size_ >= N)
	    return false;
	data_[size_++] = value;
	return true;
    }

    void clear() noexcept { size_ = 0; }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return N; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] bool full() const noexcept { return size_ == N; }

    T& operator[](std::size_t i) noexcept { return data_[i]; }
    const T& operator[](std::size_t i) const noexcept { return data_[i]; }
    [[nodiscard]] T* data() noexcept { return data_.data(); }
    [[nodiscard]] const T* data() const noexcept { return data_.data(); }

  private:
    std::array<T, N> data_{};
    std::size_t size_ = 0;
};

} // namespace rt
