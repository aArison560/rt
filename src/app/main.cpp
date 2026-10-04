#include <exception>
#include <iostream>

#include "rt/base/Log.hpp"

namespace {

constexpr const char* kVersion = "0.1.0";

int run() {
    std::cout << "rt " << kVersion << '\n';
    return 0;
}

} // namespace

// Filet unique (règle R2, T015) : seul `try/catch` autorisé du dépôt.
// Tout le hot path rapporte par `rt::Status`/`bool`, jamais par exception.
int main() {
    try {
	return run();
    } catch (const std::exception& e) {
	rt::log::error(e.what());
	return 1;
    } catch (...) {
	rt::log::error("unknown exception");
	return 1;
    }
}
