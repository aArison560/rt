// Tests de `rt::Status`, `rt::Result<T>` et `rt::log` (T015).
// Programme autonome (Catch2 en T017) : renvoie ≠ 0 si un contrôle échoue.

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "rt/base/Log.hpp"
#include "rt/base/Result.hpp"
#include "rt/base/Status.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
	if (!(cond)) {                                                                             \
	    ++g_failures;                                                                          \
	    std::cerr << "FAIL line " << __LINE__ << ": " #cond << '\n';                           \
	}                                                                                          \
    } while (false)

// --- Propagation sur 3 niveaux (Status) --------------------------------------------------
rt::Status level1Status(bool fail) {
    if (fail) {
	return RT_ERROR(rt::StatusCode::ParseError, "level1 failed");
    }
    return rt::Status::ok();
}

rt::Status level2Status(bool fail) {
    rt::Status status = level1Status(fail);
    if (status.isError()) {
	return status; // propagation sans perte (message + ligne d'origine)
    }
    return rt::Status::ok();
}

rt::Status level3Status(bool fail) {
    rt::Status status = level2Status(fail);
    if (status.isError()) {
	return status;
    }
    return rt::Status::ok();
}

// --- Propagation sur 3 niveaux (Result<int>) ---------------------------------------------
rt::Result<int> level1Result(bool fail) {
    if (fail) {
	return rt::Result<int>::fail(RT_ERROR(rt::StatusCode::IoError, "disk gone"));
    }
    return rt::Result<int>::ok(41);
}

rt::Result<int> level2Result(bool fail) {
    rt::Result<int> inner = level1Result(fail);
    if (inner.isError()) {
	return rt::Result<int>::fail(inner.status());
    }
    return rt::Result<int>::ok(inner.value() + 1);
}

rt::Result<int> level3Result(bool fail) {
    rt::Result<int> inner = level2Result(fail);
    if (inner.isError()) {
	return rt::Result<int>::fail(inner.status());
    }
    return rt::Result<int>::ok(inner.value() + 1);
}

// --- Filet façon `main` : même forme que `src/app/main.cpp` ------------------------------
int runWithFilet(bool doThrow) {
    try {
	if (doThrow) {
	    throw std::runtime_error("boom");
	}
	return 0;
    } catch (const std::exception& e) {
	rt::log::error(e.what());
	return 1;
    } catch (...) {
	rt::log::error("unknown exception");
	return 1;
    }
}

int runWithUnknownFilet(bool doThrow) {
    try {
	if (doThrow) {
	    throw 42;
	}
	return 0;
    } catch (const std::exception& e) {
	rt::log::error(e.what());
	return 1;
    } catch (...) {
	rt::log::error("unknown exception");
	return 1;
    }
}

} // namespace

int main() {
    using namespace rt;

    static_assert(std::is_move_constructible_v<Result<int>>);
    static_assert(std::is_move_constructible_v<Status>);

    // --- Status : ok vs erreur ------------------------------------------------------------
    {
	const Status ok = Status::ok();
	CHECK(ok.isOk());
	CHECK(!ok.isError());
	CHECK(ok.code == StatusCode::Ok);
    }
    {
	const Status err = RT_ERROR(StatusCode::InvalidArgument, "bad width");
	CHECK(err.isError());
	CHECK(!err.isOk());
	CHECK(err.code == StatusCode::InvalidArgument);
	CHECK(!err.message.empty()); // aucun chemin d'erreur muet
	CHECK(err.message == "bad width");
	CHECK(err.line > 0);
	CHECK(toString(err.code) == "invalid_argument");
    }
    CHECK(toString(StatusCode::Ok) == "ok");
    CHECK(toString(StatusCode::Unknown) == "unknown");

    // --- Result<T> : valeur et erreur -------------------------------------------------------
    {
	const Result<int> good = Result<int>::ok(7);
	CHECK(good.isOk());
	CHECK(good.hasValue());
	CHECK(good.value() == 7);
	CHECK((*good) == 7);
	CHECK(good.status().isOk());
	CHECK(good.valueOr(0) == 7);
    }
    {
	const Result<int> bad = Result<int>::fail(RT_ERROR(StatusCode::NotFound, "no texture"));
	CHECK(bad.isError());
	CHECK(!bad.hasValue());
	CHECK(bad.status().code == StatusCode::NotFound);
	CHECK(!bad.status().message.empty());
	CHECK(bad.valueOr(99) == 99);
    }
    {
	// Déplacement : le contenu suit, sans exception.
	Result<std::string> src = Result<std::string>::ok("hello");
	const Result<std::string> dst = std::move(src);
	CHECK(dst.isOk());
	CHECK(dst.value() == "hello");
    }
    {
	// Défaut = erreur explicite, jamais un succès silencieux sans valeur.
	const Result<int> empty;
	CHECK(empty.isError());
	CHECK(!empty.status().message.empty());
    }

    // --- Propagation 3 niveaux --------------------------------------------------------------
    {
	CHECK(level3Status(false).isOk());
	const Status err = level3Status(true);
	CHECK(err.isError());
	CHECK(err.code == StatusCode::ParseError);
	CHECK(err.message == "level1 failed"); // message d'origine préservé
	CHECK(err.line > 0);
    }
    {
	const Result<int> good = level3Result(false);
	CHECK(good.isOk());
	CHECK(good.value() == 43); // 41 + 1 + 1
    }
    {
	const Result<int> bad = level3Result(true);
	CHECK(bad.isError());
	CHECK(bad.status().code == StatusCode::IoError);
	CHECK(bad.status().message == "disk gone");
    }

    // --- Logger : niveaux et variable d'environnement ----------------------------------------
    CHECK(std::string(log::levelName(log::Level::Info)) == "info");
    CHECK(std::string(log::levelName(log::Level::Warning)) == "warning");
    CHECK(std::string(log::levelName(log::Level::Error)) == "error");
    CHECK(log::levelFromEnv() == log::Level::Info); // défaut sans RT_LOG
    ::setenv("RT_LOG", "error", 1);                 // NOLINT(misc-include-cleaner)
    CHECK(log::levelFromEnv() == log::Level::Error);
    ::setenv("RT_LOG", "warn", 1); // NOLINT(misc-include-cleaner)
    CHECK(log::levelFromEnv() == log::Level::Warning);
    ::setenv("RT_LOG", "info", 1); // NOLINT(misc-include-cleaner)
    CHECK(log::levelFromEnv() == log::Level::Info);
    ::unsetenv("RT_LOG"); // NOLINT(misc-include-cleaner)
    CHECK(log::levelFromEnv() == log::Level::Info);
    // Ces appels ne doivent ni crasher ni lancer (flux unique = stderr).
    log::info("t015 info probe");
    log::warn("t015 warn probe");
    log::error("t015 error probe");

    // --- Filet : une exception devient un code ≠ 0, jamais un crash --------------------------
    CHECK(runWithFilet(false) == 0);
    CHECK(runWithFilet(true) == 1);
    CHECK(runWithUnknownFilet(false) == 0);
    CHECK(runWithUnknownFilet(true) == 1);

    if (g_failures == 0) {
	std::cout << "test_status: OK\n";
    }
    return g_failures == 0 ? 0 : 1;
}
