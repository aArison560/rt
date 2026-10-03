#include <iostream>

namespace {
constexpr const char *kVersion = "0.1.0";
}

int main() {
    std::cout << "rt " << kVersion << std::endl;
    return 0;
}
