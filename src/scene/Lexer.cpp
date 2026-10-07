// Lexer du format `.rt` (T022) — implementation sans exception (R2).
// Voir `include/rt/scene/Lexer.hpp` pour le contrat et `docs/FORMAT_SCENE.md`
// §2 pour les regles lexicales (commentaires `#`, chaines `"..."`, nombres,
// vecteurs `(x y z)`, profondeur max 32, UTF-8 exige).

#include "rt/scene/Lexer.hpp"

#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace rt::scene {

namespace {

constexpr bool isDigit(char c) noexcept {
	return c >= '0' && c <= '9';
}

constexpr bool isAlpha(char c) noexcept {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool isIdentStart(char c) noexcept {
	return isAlpha(c) || c == '_';
}

constexpr bool isIdentChar(char c) noexcept {
	return isAlpha(c) || isDigit(c) || c == '_';
}

constexpr bool isAlnum(char c) noexcept {
	return isAlpha(c) || isDigit(c);
}

// Longueur attendue d'une sequence UTF-8 d'apres l'octet de tete.
// 0 = tete invalide (dont 0xC0/0xC1 surlongs et 0xF5-0xFF hors Unicode).
[[nodiscard]] std::size_t utf8HeadLength(unsigned char lead) noexcept {
	if (lead < 0x80U) {
		return 1;
	}
	if (lead >= 0xC2U && lead <= 0xDFU) {
		return 2;
	}
	if (lead >= 0xE0U && lead <= 0xEFU) {
		return 3;
	}
	if (lead >= 0xF0U && lead <= 0xF4U) {
		return 4;
	}
	return 0;
}

// Valide la sequence UTF-8 en `content[pos .. pos+len)` (len deduite de la
// tete) : continuations `10xxxxxx`, refus des surlongs, des substituts
// U+D800-DFFF et au-dela de U+10FFFF. `outLen` = longueur en octets si valide.
[[nodiscard]] bool validUtf8At(std::string_view content, std::size_t pos,
                               std::size_t &outLen) noexcept {
	const std::size_t left = content.size() - pos;
	const auto lead = static_cast<unsigned char>(content[pos]);
	const std::size_t len = utf8HeadLength(lead);
	if (len <= 1 || len > left) {
		return false;
	}
	for (std::size_t k = 1; k < len; ++k) {
		const auto cont = static_cast<unsigned char>(content[pos + k]);
		if (cont < 0x80U || cont > 0xBFU) {
			return false;
		}
	}
	const auto second = static_cast<unsigned char>(content[pos + 1]);
	if (lead == 0xE0U && second < 0xA0U) {
		return false;
	}
	if (lead == 0xEDU && second > 0x9FU) {
		return false;
	}
	if (len == 4) {
		if (lead == 0xF0U && second < 0x90U) {
			return false;
		}
		if (lead == 0xF4U && second > 0x8FU) {
			return false;
		}
	}
	outLen = len;
	return true;
}

[[nodiscard]] std::string formatError(std::string_view filename, int line, int col,
                                      std::string_view detail) {
	std::string message;
	message.reserve(filename.size() + detail.size() + 16U);
	message.append(filename);
	message.push_back(':');
	message.append(std::to_string(line));
	message.push_back(':');
	message.append(std::to_string(col));
	message.append(": ");
	message.append(detail);
	return message;
}

[[nodiscard]] double parseNumberValue(std::string_view lexeme) noexcept {
	double value = 0.0;
	const char *first = lexeme.data();
	const char *last = lexeme.data() + lexeme.size();
	const std::from_chars_result result = std::from_chars(first, last, value);
	if (result.ec != std::errc() || result.ptr != last) {
		return 0.0;
	}
	return value;
}

} // namespace

std::string_view toString(TokenKind kind) noexcept {
	switch (kind) {
	case TokenKind::LBrace:
		return "lbrace";
	case TokenKind::RBrace:
		return "rbrace";
	case TokenKind::LParen:
		return "lparen";
	case TokenKind::RParen:
		return "rparen";
	case TokenKind::Ident:
		return "ident";
	case TokenKind::String:
		return "string";
	case TokenKind::Number:
		return "number";
	case TokenKind::End:
		return "end";
	}
	return "unknown";
}

Result<std::vector<Token>> lexContent(std::string_view content, std::string_view filename) {
	using FailTokens = Result<std::vector<Token>>;
	std::vector<Token> tokens;
	tokens.reserve(64);

	const std::size_t size = content.size();
	std::size_t i = 0;
	int line = 1;
	int col = 1;
	int depth = 0;

	const auto failAt = [&](int failLine, int failCol, StatusCode code,
	                        std::string_view detail) -> FailTokens {
		return FailTokens::fail(
		    Status(code, formatError(filename, failLine, failCol, detail), __LINE__));
	};

	while (i < size) {
		const char current = content[i];
		const auto byte = static_cast<unsigned char>(current);

		if (current == ' ' || current == '\t' || current == '\r') {
			++i;
			++col;
			continue;
		}
		if (current == '\n') {
			++i;
			++line;
			col = 1;
			continue;
		}
		if (current == '#') {
			++i;
			++col;
			while (i < size && content[i] != '\n') {
				const auto commentByte = static_cast<unsigned char>(content[i]);
				if (commentByte < 0x80U) {
					if (commentByte < 0x20U && commentByte != 0x09U) {
						return failAt(line, col, StatusCode::ParseError,
						              "invalid control byte in comment");
					}
					++i;
					++col;
					continue;
				}
				std::size_t seqLen = 0;
				if (!validUtf8At(content, i, seqLen)) {
					return failAt(line, col, StatusCode::ParseError,
					              "invalid UTF-8 sequence in comment");
				}
				i += seqLen;
				++col;
			}
			continue;
		}
		if (current == '{') {
			++depth;
			if (depth > kMaxNesting) {
				return failAt(line, col, StatusCode::LimitExceeded,
				              "nesting too deep (limit 32)");
			}
			Token token;
			token.kind = TokenKind::LBrace;
			token.line = line;
			token.col = col;
			tokens.push_back(std::move(token));
			++i;
			++col;
			continue;
		}
		if (current == '}') {
			--depth;
			if (depth < 0) {
				return failAt(line, col, StatusCode::ParseError, "unmatched '}'");
			}
			Token token;
			token.kind = TokenKind::RBrace;
			token.line = line;
			token.col = col;
			tokens.push_back(std::move(token));
			++i;
			++col;
			continue;
		}
		if (current == '(') {
			Token token;
			token.kind = TokenKind::LParen;
			token.line = line;
			token.col = col;
			tokens.push_back(std::move(token));
			++i;
			++col;
			continue;
		}
		if (current == ')') {
			Token token;
			token.kind = TokenKind::RParen;
			token.line = line;
			token.col = col;
			tokens.push_back(std::move(token));
			++i;
			++col;
			continue;
		}
		if (current == '"') {
			const int openLine = line;
			const int openCol = col;
			++i;
			++col;
			std::string value;
			bool closed = false;
			while (i < size) {
				const char inner = content[i];
				if (inner == '"') {
					closed = true;
					++i;
					++col;
					break;
				}
				if (inner == '\n' || inner == '\r') {
					return failAt(line, col, StatusCode::ParseError,
					              "unterminated string (newline before closing quote)");
				}
				if (inner == '\\') {
					if (i + 1 >= size) {
						return failAt(line, col, StatusCode::ParseError,
						              "unterminated string (trailing backslash)");
					}
					const char escaped = content[i + 1];
					if (escaped == '"') {
						value.push_back('"');
						i += 2;
						col += 2;
						continue;
					}
					if (escaped == '\\') {
						value.push_back('\\');
						i += 2;
						col += 2;
						continue;
					}
					return failAt(line, col, StatusCode::ParseError,
					              "invalid escape (only \\\" and \\\\ allowed)");
				}
				const auto innerByte = static_cast<unsigned char>(inner);
				if (innerByte < 0x80U) {
					if (innerByte < 0x20U && innerByte != 0x09U) {
						return failAt(line, col, StatusCode::ParseError,
						              "invalid control byte in string");
					}
					value.push_back(inner);
					++i;
					++col;
					continue;
				}
				std::size_t seqLen = 0;
				if (!validUtf8At(content, i, seqLen)) {
					return failAt(line, col, StatusCode::ParseError,
					              "invalid UTF-8 sequence in string");
				}
				value.append(content.data() + i, seqLen);
				i += seqLen;
				++col;
			}
			if (!closed) {
				return failAt(openLine, openCol, StatusCode::ParseError,
				              "unterminated string (missing closing quote)");
			}
			Token token;
			token.kind = TokenKind::String;
			token.text = std::move(value);
			token.line = openLine;
			token.col = openCol;
			tokens.push_back(std::move(token));
			continue;
		}
		if (isIdentStart(current)) {
			const int startCol = col;
			std::size_t j = i + 1;
			while (j < size && isIdentChar(content[j])) {
				++j;
			}
			Token token;
			token.kind = TokenKind::Ident;
			token.text.assign(content.data() + i, j - i);
			token.line = line;
			token.col = startCol;
			tokens.push_back(std::move(token));
			col += static_cast<int>(j - i);
			i = j;
			continue;
		}

		bool maybeNumber = false;
		if (isDigit(current)) {
			maybeNumber = true;
		} else if (current == '.') {
			maybeNumber = (i + 1 < size && isDigit(content[i + 1]));
		} else if (current == '+' || current == '-') {
			if (i + 1 < size && isDigit(content[i + 1])) {
				maybeNumber = true;
			} else if (i + 1 < size && content[i + 1] == '.' && i + 2 < size &&
			           isDigit(content[i + 2])) {
				maybeNumber = true;
			}
		}
		if (maybeNumber) {
			const int startCol = col;
			std::size_t j = i;
			if (content[j] == '+' || content[j] == '-') {
				++j;
			}
			const std::size_t intStart = j;
			while (j < size && isDigit(content[j])) {
				++j;
			}
			const bool hasInt = j > intStart;
			bool hasDot = false;
			std::size_t fracCount = 0;
			if (j < size && content[j] == '.') {
				hasDot = true;
				++j;
				const std::size_t fracStart = j;
				while (j < size && isDigit(content[j])) {
					++j;
				}
				fracCount = j - fracStart;
			}
			if (!hasInt && fracCount == 0) {
				(void)hasDot;
				return failAt(line, startCol, StatusCode::ParseError, "invalid number");
			}
			if (j < size && (content[j] == 'e' || content[j] == 'E')) {
				++j;
				if (j < size && (content[j] == '+' || content[j] == '-')) {
					++j;
				}
				const std::size_t expStart = j;
				while (j < size && isDigit(content[j])) {
					++j;
				}
				if (j == expStart) {
					return failAt(line, startCol, StatusCode::ParseError,
					              "invalid number: exponent has no digits");
				}
			}
			if (j < size && (isAlnum(content[j]) || content[j] == '_' || content[j] == '.')) {
				std::size_t junk = j;
				while (junk < size &&
				       (isAlnum(content[junk]) || content[junk] == '_' || content[junk] == '.' ||
				        content[junk] == '+' || content[junk] == '-' || content[junk] == 'e' ||
				        content[junk] == 'E')) {
					++junk;
				}
				std::string preview(content.data() + i, junk - i);
				if (preview.size() > 24) {
					preview.resize(24);
				}
				return failAt(line, startCol, StatusCode::ParseError,
				              std::string("invalid number '") + preview + "'");
			}
			const std::string_view lexeme(content.data() + i, j - i);
			Token token;
			token.kind = TokenKind::Number;
			token.text.assign(lexeme);
			token.numberValue = parseNumberValue(lexeme);
			token.line = line;
			token.col = startCol;
			tokens.push_back(std::move(token));
			col += static_cast<int>(j - i);
			i = j;
			continue;
		}
		if (byte >= 0x80U) {
			std::size_t seqLen = 0;
			if (!validUtf8At(content, i, seqLen)) {
				return failAt(line, col, StatusCode::ParseError, "invalid UTF-8 byte");
			}
			return failAt(line, col, StatusCode::ParseError, "invalid character");
		}
		{
			std::string detail("invalid character '");
			detail.push_back((current >= 0x20 && current < 0x7F) ? current : '?');
			detail.push_back('\'');
			return failAt(line, col, StatusCode::ParseError, detail);
		}
	}

	if (depth > 0) {
		return FailTokens::fail(Status(StatusCode::ParseError,
		                               formatError(filename, line, col, "unclosed '{'"), __LINE__));
	}

	Token end;
	end.kind = TokenKind::End;
	end.line = line;
	end.col = col;
	tokens.push_back(std::move(end));
	return FailTokens::ok(std::move(tokens));
}

Result<std::vector<Token>> lexFile(std::string_view path) {
	using FailTokens = Result<std::vector<Token>>;
	const std::string filePath(path);
	std::error_code code;
	bool isDir = false;
	{
		std::error_code dirCode;
		isDir = std::filesystem::is_directory(filePath, dirCode);
		if (!dirCode && isDir) {
			return FailTokens::fail(Status(StatusCode::IoError, filePath + ": is a directory",
			                                __LINE__));
		}
		(void)code;
	}
	std::ifstream input(filePath, std::ios::binary);
	if (!input) {
		return FailTokens::fail(
		    Status(StatusCode::IoError, filePath + ": cannot open file", __LINE__));
	}
	std::string content((std::istreambuf_iterator<char>(input)),
	                    std::istreambuf_iterator<char>());
	if (input.bad()) {
		return FailTokens::fail(
		    Status(StatusCode::IoError, filePath + ": cannot read file", __LINE__));
	}
	return lexContent(content, path);
}

} // namespace rt::scene
