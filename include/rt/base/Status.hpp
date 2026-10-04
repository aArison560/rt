#pragma once

// Codes d'erreur du noyau `base` (T015) — header-only, sans exception.
// Règle R2 (ADR-001 §2) : le hot path ne lance jamais, il renvoie `Status`
// (ou `bool`) ; `main` est le seul filet `try/catch`. Un `Status` en erreur
// porte toujours un message non vide et la ligne d'émission (`__LINE__`
// via `RT_ERROR`) : aucun chemin d'erreur muet.

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace rt {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument = 1,
    NotFound = 2,
    OutOfRange = 3,
    Overflow = 4,
    IoError = 5,
    ParseError = 6,
    LimitExceeded = 7,
    Internal = 8,
    Unknown = 9,
};

[[nodiscard]] inline constexpr std::string_view toString(StatusCode code) noexcept {
    switch (code) {
    case StatusCode::Ok:
	return "ok";
    case StatusCode::InvalidArgument:
	return "invalid_argument";
    case StatusCode::NotFound:
	return "not_found";
    case StatusCode::OutOfRange:
	return "out_of_range";
    case StatusCode::Overflow:
	return "overflow";
    case StatusCode::IoError:
	return "io_error";
    case StatusCode::ParseError:
	return "parse_error";
    case StatusCode::LimitExceeded:
	return "limit_exceeded";
    case StatusCode::Internal:
	return "internal";
    case StatusCode::Unknown:
	return "unknown";
    }
    return "unknown";
}

struct Status {
    StatusCode code = StatusCode::Ok;
    std::string message;
    int line = 0;

    Status() = default;
    Status(StatusCode code_, std::string message_, int line_)
        : code(code_), message(std::move(message_)), line(line_) {}

    [[nodiscard]] bool isOk() const noexcept { return code == StatusCode::Ok; }
    [[nodiscard]] bool isError() const noexcept { return code != StatusCode::Ok; }

    [[nodiscard]] static Status ok() { return {}; }

    [[nodiscard]] static Status error(StatusCode code, std::string_view message, int line) {
	return {code, std::string(message), line};
    }
};

// Capture la ligne d'émission : tout site d'erreur garde sa localisation.
// Exemple : `return RT_ERROR(rt::StatusCode::ParseError, "brace unclosed");`
#define RT_ERROR(code, message) (::rt::Status::error((code), (message), __LINE__))

} // namespace rt
