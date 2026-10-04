// Tests de `rt::Status`, `rt::Result<T>` et `rt::log` (T015), Catch2 (T017).

#define _POSIX_C_SOURCE 200809L // setenv/unsetenv avec -std=c++2c strict

#include <catch2/catch_amalgamated.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "rt/base/Log.hpp"
#include "rt/base/Result.hpp"
#include "rt/base/Status.hpp"

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

TEST_CASE("Status et Result<T> sont trivialement déplaçables", "[status]") {
    static_assert(std::is_move_constructible_v<rt::Result<int>>);
    static_assert(std::is_move_constructible_v<rt::Status>);
    SUCCEED();
}

TEST_CASE("Status : ok vs erreur", "[status]") {
    using namespace rt;

    const Status ok = Status::ok();
    REQUIRE(ok.isOk());
    REQUIRE_FALSE(ok.isError());
    REQUIRE(ok.code == StatusCode::Ok);

    const Status err = RT_ERROR(StatusCode::InvalidArgument, "bad width");
    REQUIRE(err.isError());
    REQUIRE_FALSE(err.isOk());
    REQUIRE(err.code == StatusCode::InvalidArgument);
    REQUIRE_FALSE(err.message.empty()); // aucun chemin d'erreur muet
    REQUIRE(err.message == "bad width");
    REQUIRE(err.line > 0);
    REQUIRE(toString(err.code) == "invalid_argument");
    REQUIRE(toString(StatusCode::Ok) == "ok");
    REQUIRE(toString(StatusCode::Unknown) == "unknown");
}

TEST_CASE("Result<T> : valeur et erreur", "[status]") {
    using namespace rt;

    const Result<int> good = Result<int>::ok(7);
    REQUIRE(good.isOk());
    REQUIRE(good.hasValue());
    REQUIRE(good.value() == 7);
    REQUIRE((*good) == 7);
    REQUIRE(good.status().isOk());
    REQUIRE(good.valueOr(0) == 7);

    const Result<int> bad = Result<int>::fail(RT_ERROR(StatusCode::NotFound, "no texture"));
    REQUIRE(bad.isError());
    REQUIRE_FALSE(bad.hasValue());
    REQUIRE(bad.status().code == StatusCode::NotFound);
    REQUIRE_FALSE(bad.status().message.empty());
    REQUIRE(bad.valueOr(99) == 99);

    // Déplacement : le contenu suit, sans exception.
    Result<std::string> src = Result<std::string>::ok("hello");
    const Result<std::string> dst = std::move(src);
    REQUIRE(dst.isOk());
    REQUIRE(dst.value() == "hello");

    // Défaut = erreur explicite, jamais un succès silencieux sans valeur.
    const Result<int> empty;
    REQUIRE(empty.isError());
    REQUIRE_FALSE(empty.status().message.empty());
}

TEST_CASE("propagation d'erreur à travers 3 niveaux", "[status]") {
    using namespace rt;

    REQUIRE(level3Status(false).isOk());
    const Status err = level3Status(true);
    REQUIRE(err.isError());
    REQUIRE(err.code == StatusCode::ParseError);
    REQUIRE(err.message == "level1 failed"); // message d'origine préservé
    REQUIRE(err.line > 0);

    const Result<int> good = level3Result(false);
    REQUIRE(good.isOk());
    REQUIRE(good.value() == 43); // 41 + 1 + 1

    const Result<int> bad = level3Result(true);
    REQUIRE(bad.isError());
    REQUIRE(bad.status().code == StatusCode::IoError);
    REQUIRE(bad.status().message == "disk gone");
}

TEST_CASE("logger : niveaux et variable d'environnement RT_LOG", "[status]") {
    using namespace rt;

    REQUIRE(std::string(log::levelName(log::Level::Info)) == "info");
    REQUIRE(std::string(log::levelName(log::Level::Warning)) == "warning");
    REQUIRE(std::string(log::levelName(log::Level::Error)) == "error");
    REQUIRE(log::levelFromEnv() == log::Level::Info); // défaut sans RT_LOG
    ::setenv("RT_LOG", "error", 1);
    REQUIRE(log::levelFromEnv() == log::Level::Error);
    ::setenv("RT_LOG", "warn", 1);
    REQUIRE(log::levelFromEnv() == log::Level::Warning);
    ::setenv("RT_LOG", "info", 1);
    REQUIRE(log::levelFromEnv() == log::Level::Info);
    ::unsetenv("RT_LOG");
    REQUIRE(log::levelFromEnv() == log::Level::Info);
    // Ces appels ne doivent ni crasher ni lancer (flux unique = stderr).
    log::info("t017 info probe");
    log::warn("t017 warn probe");
    log::error("t017 error probe");
    SUCCEED();
}

TEST_CASE("filet : une exception devient un code ≠ 0, jamais un crash", "[status]") {
    REQUIRE(runWithFilet(false) == 0);
    REQUIRE(runWithFilet(true) == 1);
    REQUIRE(runWithUnknownFilet(false) == 0);
    REQUIRE(runWithUnknownFilet(true) == 1);
}
