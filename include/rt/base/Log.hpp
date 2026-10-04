#pragma once

// Logger minimal du noyau `base` (T015) — flux unique, niveau par variable
// d'environnement `RT_LOG`. `info`/`warn`/`error` écrivent tous sur `stderr`
// avec un préfixe ; le niveau filtre les messages trop verbeux :
// `RT_LOG=info` (défaut) tout affiche, `warn` masque `info`, `error` ne garde
// que les erreurs. Valeurs acceptées : `info`, `warn`/`warning`, `error`
// (insensible à la casse simple + `0`/`1`/`2`). Ne lance jamais (filet
// `catch(...)` interne, chemin froid hors hot path).

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace rt::log {

enum class Level : std::uint8_t {
    Info = 0,
    Warning = 1,
    Error = 2,
};

[[nodiscard]] inline std::string_view levelName(Level level) noexcept {
    switch (level) {
    case Level::Info:
	return "info";
    case Level::Warning:
	return "warning";
    case Level::Error:
	return "error";
    }
    return "unknown";
}

[[nodiscard]] inline Level levelFromEnv() noexcept {
    const char* raw = std::getenv("RT_LOG"); // NOLINT(concurrency-mt-unsafe)
    if (raw == nullptr) {
	return Level::Info;
    }
    const std::string_view value(raw);
    if (value == "error" || value == "ERROR" || value == "2") {
	return Level::Error;
    }
    if (value == "warn" || value == "WARN" || value == "warning" || value == "WARNING" ||
        value == "1") {
	return Level::Warning;
    }
    return Level::Info;
}

inline void log(Level level, std::string_view message) noexcept {
    try {
	if (static_cast<int>(level) < static_cast<int>(levelFromEnv())) {
	    return;
	}
	const std::string_view prefix = level == Level::Error     ? "[rt][error] "
	                                : level == Level::Warning ? "[rt][warn] "
	                                                          : "[rt][info] ";
	std::cerr << prefix << message << '\n';
    } catch (...) {
	// Le logging ne doit jamais faire avorter le programme : on ignore
	// volontairement toute erreur d'E/S ici (chemin froid, pas de throw).
	(void)message.size();
    }
}

inline void info(std::string_view message) noexcept { log(Level::Info, message); }

inline void warn(std::string_view message) noexcept { log(Level::Warning, message); }

inline void error(std::string_view message) noexcept { log(Level::Error, message); }

} // namespace rt::log
