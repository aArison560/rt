#pragma once

// `rt::Result<T>` (T015) — petit transport de valeur ou d'erreur, sans exception.
// Règle R2 : le hot path propage les erreurs par `Status`/`Result`, jamais par
// `throw`. `Result` est déplaçable/copiable par défaut ; l'accès à la valeur
// exige `isOk()` (aucune exception sur mauvais accès : l'appelant vérifie).
// Le `Status` en erreur garde message + ligne d'origine à travers les niveaux.

#include <optional>
#include <string>
#include <utility>

#include "rt/base/Status.hpp"

namespace rt {

template <typename T> class Result {
  public:
    Result() : status_(StatusCode::Internal, std::string("empty Result"), 0) {}

    [[nodiscard]] static Result ok(T value) {
	Result result;
	result.status_ = Status::ok();
	result.value_.emplace(std::move(value));
	return result;
    }

    [[nodiscard]] static Result fail(Status status) {
	Result result;
	result.status_ = std::move(status);
	result.value_.reset();
	return result;
    }

    [[nodiscard]] bool isOk() const noexcept { return status_.isOk() && value_.has_value(); }
    [[nodiscard]] bool isError() const noexcept { return !isOk(); }
    [[nodiscard]] const Status& status() const noexcept { return status_; }
    [[nodiscard]] bool hasValue() const noexcept { return value_.has_value(); }

    // Précondition : `isOk()` (vérifiée par l'appelant, pas d'exception) ; le
    // NOLINT porte sur la ligne fautive (clang-tidy 19 ignore un NOLINT placé
    // après l'accolade fermante).
    [[nodiscard]] const T& value() const noexcept {
	return *value_; // NOLINT(bugprone-unchecked-optional-access)
    }
    [[nodiscard]] T& value() noexcept {
	return *value_; // NOLINT(bugprone-unchecked-optional-access)
    }
    [[nodiscard]] const T& operator*() const noexcept {
	return *value_; // NOLINT(bugprone-unchecked-optional-access)
    }
    [[nodiscard]] T& operator*() noexcept {
	return *value_; // NOLINT(bugprone-unchecked-optional-access)
    }
    [[nodiscard]] const T* operator->() const noexcept { return value_.operator->(); }
    [[nodiscard]] T* operator->() noexcept { return value_.operator->(); }

    [[nodiscard]] T valueOr(T fallback) const {
	if (value_.has_value()) {
	    return *value_;
	}
	return fallback;
    }

  private:
    Status status_;
    std::optional<T> value_;
};

} // namespace rt
