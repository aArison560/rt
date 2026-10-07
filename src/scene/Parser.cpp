// Parser `.rt` -> `Scene` (T023) — descente recursive sans exception (R2).
// Toute cle passe par `schema::find()` (R1), toute valeur par `check*()`
// avant stockage (aucune allocation avant validation des bornes) ; chaque
// echec est localise `fichier:ligne:colonne`. Les blocs ouverts sont tenus
// par `Result`/`vector` : une erreur unwind proprement, sans fuite.

#include "rt/scene/Parser.hpp"

#include <cmath>
#include <string>

#include "rt/schema/Directives.hpp"
#include "rt/scene/Validator.hpp"

namespace rt::scene {

namespace {

using rt::schema::checkEnum;
using rt::schema::checkInt;
using rt::schema::checkNumber;
using rt::schema::checkRotate;
using rt::schema::checkString;
using rt::schema::find;

struct Cursor {
	const std::vector<Token>* toks = nullptr;
	std::string filename;
	std::size_t pos = 0;

	[[nodiscard]] const Token& peek() const {
		return (*toks)[pos];
	}
	[[nodiscard]] const Token& peekAt(std::size_t off) const {
		std::size_t i = pos + off;
		if (i >= toks->size()) {
			return toks->back();
		}
		return (*toks)[i];
	}
	[[nodiscard]] bool atEnd() const {
		return peek().kind == TokenKind::End;
	}
	[[nodiscard]] std::string at(std::string_view detail) const {
		return atToken(peek(), detail);
	}
	[[nodiscard]] std::string atToken(const Token& tok, std::string_view detail) const {
		std::string msg;
		msg.reserve(filename.size() + detail.size() + 16U);
		msg.append(filename);
		msg.push_back(':');
		msg.append(std::to_string(tok.line));
		msg.push_back(':');
		msg.append(std::to_string(tok.col));
		msg.append(": ");
		msg.append(detail);
		return msg;
	}
	[[nodiscard]] std::string wrap(const Token& tok, const Status& st) const {
		std::string msg;
		msg.reserve(filename.size() + st.message.size() + 16U);
		msg.append(filename);
		msg.push_back(':');
		msg.append(std::to_string(tok.line));
		msg.push_back(':');
		msg.append(std::to_string(tok.col));
		msg.append(": ");
		msg.append(st.message);
		return msg;
	}
};

[[nodiscard]] Status failAt(const Cursor& cur, StatusCode code, std::string_view detail) {
	return Status(code, cur.at(detail), __LINE__);
}

[[nodiscard]] Status failTok(const Cursor& cur, const Token& tok, StatusCode code,
                             std::string_view detail) {
	return Status(code, cur.atToken(tok, detail), __LINE__);
}

[[nodiscard]] bool isIntegerText(const std::string& text) {
	for (char c : text) {
		if (c == '.' || c == 'e' || c == 'E') {
			return false;
		}
	}
	return true;
}

[[nodiscard]] bool parseLightTypeName(const std::string& text, LightType& out) {
	if (text == "point") {
		out = LightType::Point;
		return true;
	}
	if (text == "spot") {
		out = LightType::Spot;
		return true;
	}
	if (text == "directional" || text == "dir") {
		out = LightType::Directional;
		return true;
	}
	if (text == "area") {
		out = LightType::Area;
		return true;
	}
	return false;
}

[[nodiscard]] bool parseObjectTypeName(const std::string& text, ObjectType& out) {
	if (text == "sphere") {
		out = ObjectType::Sphere;
		return true;
	}
	if (text == "plane") {
		out = ObjectType::Plane;
		return true;
	}
	if (text == "cylinder") {
		out = ObjectType::Cylinder;
		return true;
	}
	if (text == "cone") {
		out = ObjectType::Cone;
		return true;
	}
	return false;
}

// --- primitives de lecture -------------------------------------------------

[[nodiscard]] Status expect(Cursor& cur, TokenKind kind, const char* what) {
	const Token& tok = cur.peek();
	if (tok.kind != kind) {
		std::string detail("expected '");
		detail.append(what);
		detail.append("'");
		return failTok(cur, tok, StatusCode::ParseError, detail);
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status takeNumber(Cursor& cur, const Token*& out) {
	const Token& tok = cur.peek();
	if (tok.kind != TokenKind::Number) {
		std::string detail("expected number, got '");
		detail.append(toString(tok.kind));
		detail.append("'");
		if (tok.kind == TokenKind::Ident) {
			detail.append(" '");
			detail.append(tok.text);
			detail.append("'");
		}
		return failTok(cur, tok, StatusCode::ParseError, detail);
	}
	out = &tok;
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status takeString(Cursor& cur, const Token*& out) {
	const Token& tok = cur.peek();
	if (tok.kind != TokenKind::String) {
		return failTok(cur, tok, StatusCode::ParseError, "expected string");
	}
	out = &tok;
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status takeIdent(Cursor& cur, const Token*& out) {
	const Token& tok = cur.peek();
	if (tok.kind != TokenKind::Ident) {
		return failTok(cur, tok, StatusCode::ParseError, "expected identifier");
	}
	out = &tok;
	++cur.pos;
	return Status::ok();
}

// `path` = chemin schema complet (pour `find` + `checkInt`) ; valide avant retour.
[[nodiscard]] Status parseIntProp(Cursor& cur, std::string_view path, int& out) {
	const Token* tok = nullptr;
	if (Status st = takeNumber(cur, tok); st.isError()) {
		return st;
	}
	if (!isIntegerText(tok->text)) {
		std::string detail("expected integer for '");
		detail.append(path);
		detail.append("'");
		return failTok(cur, *tok, StatusCode::ParseError, detail);
	}
	long long value = static_cast<long long>(tok->numberValue);
	if (Status st = checkInt(path, value); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	out = static_cast<int>(value);
	return Status::ok();
}

[[nodiscard]] Status parseSeedProp(Cursor& cur, std::string_view path, long long& out) {
	const Token* tok = nullptr;
	if (Status st = takeNumber(cur, tok); st.isError()) {
		return st;
	}
	if (!isIntegerText(tok->text)) {
		std::string detail("expected integer for '");
		detail.append(path);
		detail.append("'");
		return failTok(cur, *tok, StatusCode::ParseError, detail);
	}
	long long value = static_cast<long long>(tok->numberValue);
	if (Status st = checkInt(path, value); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	out = value;
	return Status::ok();
}

[[nodiscard]] Status parseTextureBytesProp(Cursor& cur, std::string_view path, long long& out) {
	return parseSeedProp(cur, path, out);
}

[[nodiscard]] Status parseFloatProp(Cursor& cur, std::string_view path, float& out) {
	const Token* tok = nullptr;
	if (Status st = takeNumber(cur, tok); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, tok->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	out = static_cast<float>(tok->numberValue);
	return Status::ok();
}

[[nodiscard]] Status parseVec3Prop(Cursor& cur, std::string_view path, Vec3& out) {
	const Token& open = cur.peek();
	if (open.kind != TokenKind::LParen) {
		return failTok(cur, open, StatusCode::ParseError, "expected '(' for vec3");
	}
	const Token openTok = open;
	++cur.pos;
	const Token* tx = nullptr;
	const Token* ty = nullptr;
	const Token* tz = nullptr;
	if (Status st = takeNumber(cur, tx); st.isError()) {
		return st;
	}
	if (Status st = takeNumber(cur, ty); st.isError()) {
		return st;
	}
	if (Status st = takeNumber(cur, tz); st.isError()) {
		return st;
	}
	const Token& close = cur.peek();
	if (close.kind != TokenKind::RParen) {
		return failTok(cur, close, StatusCode::ParseError, "expected ')' (vec3 needs 3 numbers)");
	}
	++cur.pos;
	if (Status st = checkNumber(path, tx->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tx, st), __LINE__);
	}
	if (Status st = checkNumber(path, ty->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*ty, st), __LINE__);
	}
	if (Status st = checkNumber(path, tz->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tz, st), __LINE__);
	}
	(void)openTok;
	out = Vec3(static_cast<float>(tx->numberValue), static_cast<float>(ty->numberValue),
	           static_cast<float>(tz->numberValue));
	return Status::ok();
}

[[nodiscard]] Status parseFloat2Prop(Cursor& cur, std::string_view path, Vec3& out) {
	const Token* tx = nullptr;
	const Token* ty = nullptr;
	if (Status st = takeNumber(cur, tx); st.isError()) {
		return st;
	}
	if (Status st = takeNumber(cur, ty); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, tx->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tx, st), __LINE__);
	}
	if (Status st = checkNumber(path, ty->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*ty, st), __LINE__);
	}
	out = Vec3(static_cast<float>(tx->numberValue), static_cast<float>(ty->numberValue), 0.0F);
	return Status::ok();
}

[[nodiscard]] Status parseFloat3Prop(Cursor& cur, std::string_view path, Vec3& out) {
	const Token* tx = nullptr;
	const Token* ty = nullptr;
	const Token* tz = nullptr;
	if (Status st = takeNumber(cur, tx); st.isError()) {
		return st;
	}
	if (Status st = takeNumber(cur, ty); st.isError()) {
		return st;
	}
	if (Status st = takeNumber(cur, tz); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, tx->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tx, st), __LINE__);
	}
	if (Status st = checkNumber(path, ty->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*ty, st), __LINE__);
	}
	if (Status st = checkNumber(path, tz->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tz, st), __LINE__);
	}
	out = Vec3(static_cast<float>(tx->numberValue), static_cast<float>(ty->numberValue),
	           static_cast<float>(tz->numberValue));
	return Status::ok();
}

// `scale` : nombre unique (uniforme) ou vecteur `(x y z)`.
[[nodiscard]] Status parseScaleProp(Cursor& cur, std::string_view path, Vec3& out) {
	if (cur.peek().kind == TokenKind::LParen) {
		return parseVec3Prop(cur, path, out);
	}
	const Token* tok = nullptr;
	if (Status st = takeNumber(cur, tok); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, tok->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	const auto v = static_cast<float>(tok->numberValue);
	out = Vec3(v, v, v);
	return Status::ok();
}

// `rotate axis x|y|z angle d`.
[[nodiscard]] Status parseRotateProp(Cursor& cur, std::string_view path, TransformOp& out) {
	const Token* axisKw = nullptr;
	if (Status st = takeIdent(cur, axisKw); st.isError()) {
		return st;
	}
	if (axisKw->text != "axis") {
		return failTok(cur, *axisKw, StatusCode::ParseError, "expected 'axis' after 'rotate'");
	}
	const Token* axisName = nullptr;
	if (Status st = takeIdent(cur, axisName); st.isError()) {
		return st;
	}
	const Token* angleKw = nullptr;
	if (Status st = takeIdent(cur, angleKw); st.isError()) {
		return st;
	}
	if (angleKw->text != "angle") {
		return failTok(cur, *angleKw, StatusCode::ParseError, "expected 'angle' after rotate axis");
	}
	const Token* angleTok = nullptr;
	if (Status st = takeNumber(cur, angleTok); st.isError()) {
		return st;
	}
	if (Status st = checkRotate(path, axisName->text, angleTok->numberValue); st.isError()) {
		return Status(st.code, cur.wrap(*angleTok, st), __LINE__);
	}
	out.kind = TransformOp::Kind::Rotate;
	out.rotateAxis = axisName->text[0];
	out.rotateAngle = static_cast<float>(angleTok->numberValue);
	return Status::ok();
}

[[nodiscard]] Status parseEnumProp(Cursor& cur, std::string_view path, std::string& out) {
	const Token* tok = nullptr;
	if (Status st = takeIdent(cur, tok); st.isError()) {
		return failTok(cur, cur.peek(), StatusCode::ParseError, "expected enum value");
	}
	if (Status st = checkEnum(path, tok->text); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	out.assign(tok->text);
	return Status::ok();
}

[[nodiscard]] Status parseStringProp(Cursor& cur, std::string_view path, std::string& out) {
	const Token* tok = nullptr;
	if (Status st = takeString(cur, tok); st.isError()) {
		return st;
	}
	if (Status st = checkString(path, tok->text); st.isError()) {
		return Status(st.code, cur.wrap(*tok, st), __LINE__);
	}
	out.assign(tok->text);
	return Status::ok();
}

// --- blocs -----------------------------------------------------------------

[[nodiscard]] Status parseLimits(Cursor& cur, Limits& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	bool seenWidth = false;
	bool seenHeight = false;
	bool seenSamples = false;
	bool seenDepth = false;
	bool seenSeed = false;
	bool seenMaxObjects = false;
	bool seenMaxLights = false;
	bool seenMaxBytes = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in limits");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.limits.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "width") {
			if (seenWidth) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'width'");
			}
			seenWidth = true;
			if (Status st = parseIntProp(cur, path, out.width); st.isError()) {
				return st;
			}
		} else if (key->text == "height") {
			if (seenHeight) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'height'");
			}
			seenHeight = true;
			if (Status st = parseIntProp(cur, path, out.height); st.isError()) {
				return st;
			}
		} else if (key->text == "samples") {
			if (seenSamples) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'samples'");
			}
			seenSamples = true;
			if (Status st = parseIntProp(cur, path, out.samples); st.isError()) {
				return st;
			}
		} else if (key->text == "max_depth") {
			if (seenDepth) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'max_depth'");
			}
			seenDepth = true;
			if (Status st = parseIntProp(cur, path, out.maxDepth); st.isError()) {
				return st;
			}
		} else if (key->text == "seed") {
			if (seenSeed) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'seed'");
			}
			seenSeed = true;
			if (Status st = parseSeedProp(cur, path, out.seed); st.isError()) {
				return st;
			}
		} else if (key->text == "max_objects") {
			if (seenMaxObjects) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'max_objects'");
			}
			seenMaxObjects = true;
			if (Status st = parseIntProp(cur, path, out.maxObjects); st.isError()) {
				return st;
			}
		} else if (key->text == "max_lights") {
			if (seenMaxLights) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'max_lights'");
			}
			seenMaxLights = true;
			if (Status st = parseIntProp(cur, path, out.maxLights); st.isError()) {
				return st;
			}
		} else if (key->text == "max_texture_bytes") {
			if (seenMaxBytes) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate 'max_texture_bytes'");
			}
			seenMaxBytes = true;
			if (Status st = parseTextureBytesProp(cur, path, out.maxTextureBytes); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseCamera(Cursor& cur, Camera& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	bool seenPos = false;
	bool seenTarget = false;
	bool seenUp = false;
	bool seenFov = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in camera");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.camera.");
		path.append(key->text);
		const auto* entry = find(path);
		if (entry == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		std::string canon(entry->path);
		if (canon == "scene.camera.position") {
			if (seenPos) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate camera position");
			}
			seenPos = true;
			if (Status st = parseVec3Prop(cur, canon, out.position); st.isError()) {
				return st;
			}
		} else if (canon == "scene.camera.target") {
			if (seenTarget) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate camera target");
			}
			seenTarget = true;
			if (Status st = parseVec3Prop(cur, canon, out.target); st.isError()) {
				return st;
			}
		} else if (canon == "scene.camera.up") {
			if (seenUp) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate camera up");
			}
			seenUp = true;
			if (Status st = parseVec3Prop(cur, canon, out.up); st.isError()) {
				return st;
			}
		} else if (canon == "scene.camera.fov") {
			if (seenFov) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate camera fov");
			}
			seenFov = true;
			if (Status st = parseFloatProp(cur, canon, out.fov); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseBackground(Cursor& cur, Background& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	bool seen = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in background");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.background.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text != "color") {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (seen) {
			return failTok(cur, *key, StatusCode::ParseError, "duplicate background color");
		}
		seen = true;
		if (Status st = parseVec3Prop(cur, path, out.color); st.isError()) {
			return st;
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseAmbient(Cursor& cur, Ambient& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	bool seenColor = false;
	bool seenIntensity = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in ambient");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.ambient.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "color") {
			if (seenColor) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate ambient color");
			}
			seenColor = true;
			if (Status st = parseVec3Prop(cur, path, out.color); st.isError()) {
				return st;
			}
		} else if (key->text == "intensity") {
			if (seenIntensity) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate ambient intensity");
			}
			seenIntensity = true;
			if (Status st = parseFloatProp(cur, path, out.intensity); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseLightBody(Cursor& cur, Light& out) {
	// `out` porte deja la tete (type/nom) ; le contenu gagne en cas de conflit
	// (FORMAT §4) : un seul `type`/`name` dans le corps ecrase la tete sans
	// erreur, le deuxieme est un doublon.
	bool seenPropType = false;
	bool seenName = false;
	bool seenPos = out.hasPosition;
	bool seenColor = false;
	bool seenIntensity = false;
	bool seenDir = out.hasDirection;
	bool seenTarget = out.hasTarget;
	bool seenAngle = false;
	bool seenSize = false;
	bool seenAtt = false;
	bool seenRange = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in light");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.lights.light.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "type") {
			if (seenPropType) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light type");
			}
			seenPropType = true;
			std::string value;
			if (Status st = parseEnumProp(cur, path, value); st.isError()) {
				return st;
			}
			LightType parsed = LightType::Point;
			if (!parseLightTypeName(value, parsed)) {
				return failTok(cur, *key, StatusCode::InvalidArgument, "bad light type");
			}
			if (value == "dir") {
				parsed = LightType::Directional;
			}
			out.type = parsed;
		} else if (key->text == "name") {
			if (seenName) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light name");
			}
			seenName = true;
			if (Status st = parseStringProp(cur, path, out.name); st.isError()) {
				return st;
			}
		} else if (key->text == "position") {
			if (seenPos) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light position");
			}
			seenPos = true;
			if (Status st = parseVec3Prop(cur, path, out.position); st.isError()) {
				return st;
			}
			out.hasPosition = true;
		} else if (key->text == "color") {
			if (seenColor) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light color");
			}
			seenColor = true;
			if (Status st = parseVec3Prop(cur, path, out.color); st.isError()) {
				return st;
			}
		} else if (key->text == "intensity") {
			if (seenIntensity) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light intensity");
			}
			seenIntensity = true;
			if (Status st = parseFloatProp(cur, path, out.intensity); st.isError()) {
				return st;
			}
		} else if (key->text == "direction") {
			if (seenDir) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light direction");
			}
			seenDir = true;
			if (Status st = parseVec3Prop(cur, path, out.direction); st.isError()) {
				return st;
			}
			out.hasDirection = true;
		} else if (key->text == "target") {
			if (seenTarget) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light target");
			}
			seenTarget = true;
			if (Status st = parseVec3Prop(cur, path, out.target); st.isError()) {
				return st;
			}
			out.hasTarget = true;
		} else if (key->text == "angle") {
			if (seenAngle) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light angle");
			}
			seenAngle = true;
			if (Status st = parseFloatProp(cur, path, out.angle); st.isError()) {
				return st;
			}
		} else if (key->text == "size") {
			if (seenSize) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light size");
			}
			seenSize = true;
			if (Status st = parseFloat2Prop(cur, path, out.size); st.isError()) {
				return st;
			}
			out.hasSize = true;
		} else if (key->text == "attenuation") {
			if (seenAtt) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light attenuation");
			}
			seenAtt = true;
			if (Status st = parseFloat3Prop(cur, path, out.attenuation); st.isError()) {
				return st;
			}
		} else if (key->text == "range") {
			if (seenRange) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate light range");
			}
			seenRange = true;
			if (Status st = parseFloatProp(cur, path, out.range); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

// Tete `light [type] ["nom"]` : les deux formes sont equivalentes au contenu.
[[nodiscard]] Status parseLightHead(Cursor& cur, Light& out) {
	// `light` deja consomme par l'appelant.
	if (cur.peek().kind == TokenKind::Ident) {
		const std::string& text = cur.peek().text;
		LightType parsed = LightType::Point;
		if (parseLightTypeName(text, parsed)) {
			std::string path("scene.lights.light.type");
			if (Status st = checkEnum(path, text == "dir" ? "dir" : text); st.isError()) {
				return Status(st.code, cur.wrap(cur.peek(), st), __LINE__);
			}
			out.type = parsed;
			++cur.pos;
		}
	}
	if (cur.peek().kind == TokenKind::String) {
		const Token& nameTok = cur.peek();
		if (Status st = checkString("scene.lights.light.name", nameTok.text); st.isError()) {
			return Status(st.code, cur.wrap(nameTok, st), __LINE__);
		}
		out.name.assign(nameTok.text);
		++cur.pos;
	}
	return Status::ok();
}

[[nodiscard]] Status parseTexture(Cursor& cur, TextureRef& out) {
	// `texture` deja consomme ; tete `"fichier"` requise (FORMAT §4).
	const Token* fileTok = nullptr;
	if (Status st = takeString(cur, fileTok); st.isError()) {
		return failTok(cur, cur.peek(), StatusCode::ParseError,
		               "expected texture file string after 'texture'");
	}
	if (Status st = checkString("scene.objects.object.material.texture.file", fileTok->text);
	    st.isError()) {
		return Status(st.code, cur.wrap(*fileTok, st), __LINE__);
	}
	out.file.assign(fileTok->text);
	out.present = true;
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	bool seenScale = false;
	bool seenOffset = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in texture");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.objects.object.material.texture.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "scale") {
			if (seenScale) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate texture scale");
			}
			seenScale = true;
			if (Status st = parseFloat2Prop(cur, path, out.scale); st.isError()) {
				return st;
			}
		} else if (key->text == "offset") {
			if (seenOffset) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate texture offset");
			}
			seenOffset = true;
			if (Status st = parseFloat2Prop(cur, path, out.offset); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parsePattern(Cursor& cur, PatternRef& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	out.present = true;
	bool seenType = false;
	bool seenScale = false;
	bool seenFreq = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in pattern");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.objects.object.material.pattern.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "type") {
			if (seenType) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate pattern type");
			}
			seenType = true;
			if (Status st = parseEnumProp(cur, path, out.type); st.isError()) {
				return st;
			}
		} else if (key->text == "scale") {
			if (seenScale) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate pattern scale");
			}
			seenScale = true;
			if (Status st = parseFloatProp(cur, path, out.scale); st.isError()) {
				return st;
			}
		} else if (key->text == "frequency") {
			if (seenFreq) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate pattern frequency");
			}
			seenFreq = true;
			if (Status st = parseFloatProp(cur, path, out.frequency); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseMaterialBody(Cursor& cur, Material& out) {
	bool seenAlbedo = false;
	bool seenAmbient = false;
	bool seenDiffuse = false;
	bool seenSpec = false;
	bool seenShiny = false;
	bool seenRefl = false;
	bool seenTrans = false;
	bool seenIor = false;
	bool seenBump = false;
	bool seenTex = false;
	bool seenPat = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in material");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		if (key->text == "texture") {
			if (seenTex) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material texture");
			}
			seenTex = true;
			if (Status st = parseTexture(cur, out.texture); st.isError()) {
				return st;
			}
			continue;
		}
		if (key->text == "pattern") {
			if (seenPat) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material pattern");
			}
			seenPat = true;
			if (Status st = parsePattern(cur, out.pattern); st.isError()) {
				return st;
			}
			continue;
		}
		std::string path("scene.objects.object.material.");
		path.append(key->text);
		const auto* entry = find(path);
		if (entry == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		std::string canon(entry->path);
		if (canon == "scene.objects.object.material.albedo") {
			if (seenAlbedo) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material albedo");
			}
			seenAlbedo = true;
			if (Status st = parseVec3Prop(cur, canon, out.albedo); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.ambient") {
			if (seenAmbient) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material ambient");
			}
			seenAmbient = true;
			if (Status st = parseFloatProp(cur, canon, out.ambient); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.diffuse") {
			if (seenDiffuse) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material diffuse");
			}
			seenDiffuse = true;
			if (Status st = parseFloatProp(cur, canon, out.diffuse); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.specular") {
			if (seenSpec) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material specular");
			}
			seenSpec = true;
			if (Status st = parseFloatProp(cur, canon, out.specular); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.shininess") {
			if (seenShiny) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material shininess");
			}
			seenShiny = true;
			if (Status st = parseFloatProp(cur, canon, out.shininess); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.reflectivity") {
			if (seenRefl) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material reflectivity");
			}
			seenRefl = true;
			if (Status st = parseFloatProp(cur, canon, out.reflectivity); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.transparency") {
			if (seenTrans) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material transparency");
			}
			seenTrans = true;
			if (Status st = parseFloatProp(cur, canon, out.transparency); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.ior") {
			if (seenIor) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material ior");
			}
			seenIor = true;
			if (Status st = parseFloatProp(cur, canon, out.ior); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.material.bump") {
			if (seenBump) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate material bump");
			}
			seenBump = true;
			if (Status st = parseFloatProp(cur, canon, out.bump); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseTransformBody(Cursor& cur, std::string_view basePath, Transform& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in transform");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path(basePath);
		path.push_back('.');
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "translate") {
			TransformOp op;
			op.kind = TransformOp::Kind::Translate;
			if (Status st = parseVec3Prop(cur, path, op.translate); st.isError()) {
				return st;
			}
			out.ops.push_back(op);
		} else if (key->text == "scale") {
			TransformOp op;
			op.kind = TransformOp::Kind::Scale;
			if (Status st = parseScaleProp(cur, path, op.scale); st.isError()) {
				return st;
			}
			out.ops.push_back(op);
		} else if (key->text == "rotate") {
			TransformOp op;
			if (Status st = parseRotateProp(cur, path, op); st.isError()) {
				return st;
			}
			out.ops.push_back(op);
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseSliceBody(Cursor& cur, Slice& out) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	out.present = true;
	bool seenAxis = false;
	bool seenMin = false;
	bool seenMax = false;
	bool seenFrame = false;
	bool seenShape = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in slice");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		std::string path("scene.objects.object.slice.");
		path.append(key->text);
		if (find(path) == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		if (key->text == "axis") {
			if (seenAxis) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate slice axis");
			}
			seenAxis = true;
			if (Status st = parseEnumProp(cur, path, out.axis); st.isError()) {
				return st;
			}
		} else if (key->text == "min") {
			if (seenMin) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate slice min");
			}
			seenMin = true;
			float v = 0.0F;
			if (Status st = parseFloatProp(cur, path, v); st.isError()) {
				// `min` n'a pas de borne : le schema n'a pas de min/max ;
				// `checkNumber` passe (pas de range) si le type est bon.
				// Si le type est incoherent, l'erreur remonte ici.
				return st;
			}
			out.minValue = v;
			out.hasMin = true;
		} else if (key->text == "max") {
			if (seenMax) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate slice max");
			}
			seenMax = true;
			float v = 0.0F;
			if (Status st = parseFloatProp(cur, path, v); st.isError()) {
				return st;
			}
			out.maxValue = v;
			out.hasMax = true;
		} else if (key->text == "frame") {
			if (seenFrame) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate slice frame");
			}
			seenFrame = true;
			if (Status st = parseEnumProp(cur, path, out.frame); st.isError()) {
				return st;
			}
		} else if (key->text == "shape") {
			if (seenShape) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate slice shape");
			}
			seenShape = true;
			if (Status st = parseEnumProp(cur, path, out.shape); st.isError()) {
				return st;
			}
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseObjectBody(Cursor& cur, Object& out) {
	// Tete porte deja type/nom ; le contenu gagne une fois (FORMAT §4),
	// le deuxieme `type`/`name` dans le corps est un doublon.
	bool seenType = false;
	bool seenName = false;
	bool seenCenter = false;
	bool seenRadius = false;
	bool seenPoint = false;
	bool seenNormal = false;
	bool seenAxis = false;
	bool seenAngle = false;
	bool seenHeight = false;
	bool seenMaterial = false;
	bool seenTransform = false;
	bool seenSlice = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in object");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		if (key->text == "material") {
			// Reference `material "verre"` (sans bloc) ou bloc inline.
			if (cur.peek().kind == TokenKind::String &&
			    cur.peekAt(1).kind != TokenKind::LBrace) {
				if (seenMaterial) {
					return failTok(cur, *key, StatusCode::ParseError,
					               "duplicate object material");
				}
				seenMaterial = true;
				const Token& refTok = cur.peek();
				if (Status st = checkString("scene.objects.object.name", refTok.text);
				    st.isError()) {
					return Status(st.code, cur.wrap(refTok, st), __LINE__);
				}
				out.material.materialRef.assign(refTok.text);
				++cur.pos;
				continue;
			}
			if (cur.peek().kind == TokenKind::String) {
				// `material "nom" { ... }` : nom ignore, bloc parse.
				++cur.pos;
			}
			if (seenMaterial) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object material");
			}
			seenMaterial = true;
			if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
				return st;
			}
			if (Status st = parseMaterialBody(cur, out.material); st.isError()) {
				return st;
			}
			continue;
		}
		if (key->text == "transform") {
			if (seenTransform) {
				return failTok(cur, *key, StatusCode::ParseError,
				               "duplicate object transform");
			}
			seenTransform = true;
			if (Status st = parseTransformBody(cur, "scene.objects.object.transform",
			                                   out.transform);
			    st.isError()) {
				return st;
			}
			continue;
		}
		if (key->text == "slice") {
			if (seenSlice) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object slice");
			}
			seenSlice = true;
			if (Status st = parseSliceBody(cur, out.slice); st.isError()) {
				return st;
			}
			continue;
		}
		std::string path("scene.objects.object.");
		path.append(key->text);
		const auto* entry = find(path);
		if (entry == nullptr) {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		std::string canon(entry->path);
		if (canon == "scene.objects.object.type") {
			if (seenType) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object type");
			}
			seenType = true;
			std::string value;
			if (Status st = parseEnumProp(cur, path, value); st.isError()) {
				return st;
			}
			ObjectType parsed = ObjectType::Sphere;
			if (!parseObjectTypeName(value, parsed)) {
				return failTok(cur, *key, StatusCode::InvalidArgument, "bad object type");
			}
			out.type = parsed;
			out.hasType = true;
		} else if (canon == "scene.objects.object.name") {
			if (seenName) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object name");
			}
			seenName = true;
			if (Status st = parseStringProp(cur, path, out.name); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.center") {
			if (seenCenter) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object center");
			}
			seenCenter = true;
			if (Status st = parseVec3Prop(cur, canon, out.center); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.radius") {
			if (seenRadius) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object radius");
			}
			seenRadius = true;
			if (Status st = parseFloatProp(cur, canon, out.radius); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.point") {
			if (seenPoint) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object point");
			}
			seenPoint = true;
			if (Status st = parseVec3Prop(cur, canon, out.point); st.isError()) {
				return st;
			}
			out.hasPoint = true;
		} else if (canon == "scene.objects.object.normal") {
			if (seenNormal) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object normal");
			}
			seenNormal = true;
			if (Status st = parseVec3Prop(cur, canon, out.normal); st.isError()) {
				return st;
			}
			out.hasNormal = true;
		} else if (canon == "scene.objects.object.axis") {
			if (seenAxis) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object axis");
			}
			seenAxis = true;
			if (Status st = parseVec3Prop(cur, canon, out.axis); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.angle") {
			if (seenAngle) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object angle");
			}
			seenAngle = true;
			if (Status st = parseFloatProp(cur, canon, out.angle); st.isError()) {
				return st;
			}
		} else if (canon == "scene.objects.object.height") {
			if (seenHeight) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate object height");
			}
			seenHeight = true;
			if (Status st = parseFloatProp(cur, canon, out.height); st.isError()) {
				return st;
			}
			out.hasHeight = true;
		} else {
			std::string detail("unknown directive: ");
			detail.append(path);
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	if (!out.hasType) {
		const Token& where = cur.peek();
		return failTok(cur, where, StatusCode::ParseError, "object missing required 'type'");
	}
	return Status::ok();
}

[[nodiscard]] Status parseObjectHead(Cursor& cur, Object& out) {
	if (cur.peek().kind == TokenKind::Ident) {
		const std::string& text = cur.peek().text;
		ObjectType parsed = ObjectType::Sphere;
		if (parseObjectTypeName(text, parsed)) {
			std::string path("scene.objects.object.type");
			if (Status st = checkEnum(path, text); st.isError()) {
				return Status(st.code, cur.wrap(cur.peek(), st), __LINE__);
			}
			out.type = parsed;
			out.hasType = true;
			++cur.pos;
		}
	}
	if (cur.peek().kind == TokenKind::String) {
		const Token& nameTok = cur.peek();
		if (Status st = checkString("scene.objects.object.name", nameTok.text); st.isError()) {
			return Status(st.code, cur.wrap(nameTok, st), __LINE__);
		}
		out.name.assign(nameTok.text);
		++cur.pos;
	}
	return Status::ok();
}

[[nodiscard]] Status parseGroupBody(Cursor& cur, Group& out, int depth);

[[nodiscard]] Status parseGroupBody(Cursor& cur, Group& out, int depth) {
	if (depth > kMaxNesting) {
		return failAt(cur, StatusCode::LimitExceeded, "group nesting too deep (limit 32)");
	}
	// Tete porte deja le nom ; le contenu gagne une fois (FORMAT §4).
	bool seenName = false;
	bool seenTransform = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in group");
		}
		if (cur.peek().kind != TokenKind::Ident) {
			return failTok(cur, cur.peek(), StatusCode::ParseError, "expected 'object', 'group', "
			                                                        "'name' or 'transform'");
		}
		const std::string& kw = cur.peek().text;
		if (kw == "object") {
			const Token headTok = cur.peek();
			++cur.pos;
			Object obj;
			if (Status st = parseObjectHead(cur, obj); st.isError()) {
				return st;
			}
			if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
				return failTok(cur, headTok, StatusCode::ParseError,
				               "expected '{' after 'object'");
			}
			if (Status st = parseObjectBody(cur, obj); st.isError()) {
				return st;
			}
			if (out.objects.size() >= static_cast<std::size_t>(kHardMaxObjects)) {
				return failTok(cur, headTok, StatusCode::LimitExceeded,
				               "scene too large: too many objects (hard limit 100000)");
			}
			out.objects.push_back(std::move(obj));
			continue;
		}
		if (kw == "group") {
			const Token headTok = cur.peek();
			(void)headTok;
			++cur.pos;
			auto child = std::make_shared<Group>();
			if (cur.peek().kind == TokenKind::String) {
				const Token& nameTok = cur.peek();
				if (Status st =
				        checkString("scene.objects.group.name", nameTok.text);
				    st.isError()) {
					return Status(st.code, cur.wrap(nameTok, st), __LINE__);
				}
				child->name.assign(nameTok.text);
				++cur.pos;
			}
			if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
				return st;
			}
			if (Status st = parseGroupBody(cur, *child, depth + 1); st.isError()) {
				return st;
			}
			if (out.children.size() >= static_cast<std::size_t>(kHardMaxObjects)) {
				return failTok(cur, cur.peek(), StatusCode::LimitExceeded,
				               "scene too large: too many groups (hard limit 100000)");
			}
			out.children.push_back(std::move(child));
			continue;
		}
		if (kw == "name") {
			const Token* key = nullptr;
			if (Status st = takeIdent(cur, key); st.isError()) {
				return st;
			}
			if (seenName) {
				return failTok(cur, *key, StatusCode::ParseError, "duplicate group name");
			}
			seenName = true;
			if (Status st = parseStringProp(cur, "scene.objects.group.name", out.name);
			    st.isError()) {
				return st;
			}
			continue;
		}
		if (kw == "transform") {
			const Token* key = nullptr;
			if (Status st = takeIdent(cur, key); st.isError()) {
				return st;
			}
			if (seenTransform) {
				return failTok(cur, *key, StatusCode::ParseError,
				               "duplicate group transform");
			}
			seenTransform = true;
			if (Status st = parseTransformBody(cur, "scene.objects.group.transform",
			                                   out.transform);
			    st.isError()) {
				return st;
			}
			continue;
		}
		std::string detail("unknown directive in group: '");
		detail.append(kw);
		detail.append("'");
		return failTok(cur, cur.peek(), StatusCode::NotFound, detail);
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseObjects(Cursor& cur, Scene& scene) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in objects");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		if (key->text == "object") {
			if (scene.objects.size() >= static_cast<std::size_t>(kHardMaxObjects)) {
				return failTok(cur, *key, StatusCode::LimitExceeded,
				               "scene too large: too many objects (hard limit 100000)");
			}
			Object obj;
			if (Status st = parseObjectHead(cur, obj); st.isError()) {
				return st;
			}
			if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
				return failTok(cur, *key, StatusCode::ParseError,
				               "expected '{' after 'object'");
			}
			if (Status st = parseObjectBody(cur, obj); st.isError()) {
				return st;
			}
			scene.objects.push_back(std::move(obj));
		} else if (key->text == "group") {
			if (scene.groups.size() >= static_cast<std::size_t>(kHardMaxObjects)) {
				return failTok(cur, *key, StatusCode::LimitExceeded,
				               "scene too large: too many groups (hard limit 100000)");
			}
			Group group;
			if (cur.peek().kind == TokenKind::String) {
				const Token& nameTok = cur.peek();
				if (Status st =
				        checkString("scene.objects.group.name", nameTok.text);
				    st.isError()) {
					return Status(st.code, cur.wrap(nameTok, st), __LINE__);
				}
				group.name.assign(nameTok.text);
				++cur.pos;
			}
			if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
				return failTok(cur, *key, StatusCode::ParseError,
				               "expected '{' after 'group'");
			}
			if (Status st = parseGroupBody(cur, group, 1); st.isError()) {
				return st;
			}
			scene.groups.push_back(std::move(group));
		} else {
			std::string detail("unknown directive in objects: '");
			detail.append(key->text);
			detail.append("'");
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Status parseLights(Cursor& cur, Scene& scene) {
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return st;
	}
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return failAt(cur, StatusCode::ParseError, "unclosed '{' in lights");
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return st;
		}
		if (key->text != "light") {
			std::string detail("unknown directive in lights: '");
			detail.append(key->text);
			detail.append("'");
			return failTok(cur, *key, StatusCode::NotFound, detail);
		}
		Light light;
		if (Status st = parseLightHead(cur, light); st.isError()) {
			return st;
		}
		if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
			return failTok(cur, *key, StatusCode::ParseError, "expected '{' after 'light'");
		}
		if (Status st = parseLightBody(cur, light); st.isError()) {
			return st;
		}
		if (scene.lights.size() >= static_cast<std::size_t>(kHardMaxLights)) {
			return failTok(cur, *key, StatusCode::LimitExceeded,
			               "scene too large: too many lights (hard limit 1024)");
		}
		scene.lights.push_back(std::move(light));
	}
	++cur.pos;
	return Status::ok();
}

[[nodiscard]] Result<Scene> parseSceneTokens(Cursor& cur) {
	using FailScene = Result<Scene>;
	// `scene ["nom"] { ... }` — legacy (sans `scene`) refuse proprement.
	if (cur.atEnd()) {
		return FailScene::fail(failAt(cur, StatusCode::ParseError,
		                              "empty file: expected 'scene' block "
		                              "(legacy format not supported, convert)"));
	}
	const Token& first = cur.peek();
	if (first.kind != TokenKind::Ident || first.text != "scene") {
		std::string detail("expected 'scene' block");
		if (first.kind == TokenKind::Ident) {
			detail.append(", got '");
			detail.append(first.text);
			detail.append("'");
		}
		detail.append(" (legacy format not supported, convert)");
		return FailScene::fail(failTok(cur, first, StatusCode::ParseError, detail));
	}
	const Token sceneTok = first;
	++cur.pos;
	Scene scene;
	scene.init();
	if (cur.peek().kind == TokenKind::String) {
		const Token& nameTok = cur.peek();
		scene.name.assign(nameTok.text);
		++cur.pos;
	}
	if (Status st = expect(cur, TokenKind::LBrace, "{"); st.isError()) {
		return FailScene::fail(st);
	}
	bool seenLimits = false;
	bool seenCamera = false;
	bool seenBackground = false;
	bool seenAmbient = false;
	bool seenLights = false;
	bool seenObjects = false;
	while (cur.peek().kind != TokenKind::RBrace) {
		if (cur.atEnd()) {
			return FailScene::fail(
			    failTok(cur, sceneTok, StatusCode::ParseError, "unclosed '{' in scene"));
		}
		const Token* key = nullptr;
		if (Status st = takeIdent(cur, key); st.isError()) {
			return FailScene::fail(st);
		}
		if (key->text == "limits") {
			if (seenLimits) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'limits'"));
			}
			seenLimits = true;
			if (Status st = parseLimits(cur, scene.limits); st.isError()) {
				return FailScene::fail(st);
			}
		} else if (key->text == "camera") {
			if (seenCamera) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'camera'"));
			}
			seenCamera = true;
			if (Status st = parseCamera(cur, scene.camera); st.isError()) {
				return FailScene::fail(st);
			}
		} else if (key->text == "background") {
			if (seenBackground) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'background'"));
			}
			seenBackground = true;
			if (Status st = parseBackground(cur, scene.background); st.isError()) {
				return FailScene::fail(st);
			}
		} else if (key->text == "ambient") {
			if (seenAmbient) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'ambient'"));
			}
			seenAmbient = true;
			if (Status st = parseAmbient(cur, scene.ambient); st.isError()) {
				return FailScene::fail(st);
			}
		} else if (key->text == "lights") {
			if (seenLights) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'lights'"));
			}
			seenLights = true;
			if (Status st = parseLights(cur, scene); st.isError()) {
				return FailScene::fail(st);
			}
		} else if (key->text == "objects") {
			if (seenObjects) {
				return FailScene::fail(
				    failTok(cur, *key, StatusCode::ParseError, "duplicate 'objects'"));
			}
			seenObjects = true;
			if (Status st = parseObjects(cur, scene); st.isError()) {
				return FailScene::fail(st);
			}
		} else {
			std::string detail("unknown directive: scene.");
			detail.append(key->text);
			return FailScene::fail(failTok(cur, *key, StatusCode::NotFound, detail));
		}
	}
	const Token closeTok = cur.peek();
	++cur.pos;
	if (!seenCamera) {
		return FailScene::fail(
		    failTok(cur, closeTok, StatusCode::ParseError, "missing required block 'camera'"));
	}
	if (!seenObjects) {
		return FailScene::fail(
		    failTok(cur, closeTok, StatusCode::ParseError, "missing required block 'objects'"));
	}
	if (!cur.atEnd()) {
		return FailScene::fail(
		    failTok(cur, cur.peek(), StatusCode::ParseError, "expected end of file"));
	}
	// Passe de validation T024 : bornes du schema + `limits` + croisee.
	// Le controle a lieu apres le parse (l'ordre des sous-blocs est libre)
	// mais avant tout usage ; les vecteurs n'ont grandi que par `push_back`
	// borne par la taille du fichier et les garde-fous durs (pas de
	// `reserve(max)` sur entree non validee, cf. MEMORY_STRATEGY.md §2).
	if (Status vst = validate(scene); vst.isError()) {
		return FailScene::fail(failTok(cur, closeTok, vst.code, vst.message));
	}
	scene.touchObjects();
	scene.markClean();
	return FailScene::ok(std::move(scene));
}

} // namespace

Result<Scene> parseTokens(const std::vector<Token>& tokens, std::string_view filename) {
	if (tokens.empty()) {
		return Result<Scene>::fail(
		    Status(StatusCode::ParseError, std::string(filename) + ":0:0: empty token stream",
		           __LINE__));
	}
	Cursor cur;
	cur.toks = &tokens;
	cur.filename.assign(filename);
	cur.pos = 0;
	return parseSceneTokens(cur);
}

Result<Scene> parseContent(std::string_view content, std::string_view filename) {
	Result<std::vector<Token>> lexed = lexContent(content, filename);
	if (lexed.isError()) {
		return Result<Scene>::fail(lexed.status());
	}
	return parseTokens(lexed.value(), filename);
}

Result<Scene> parseFile(std::string_view path) {
	Result<std::vector<Token>> lexed = lexFile(path);
	if (lexed.isError()) {
		return Result<Scene>::fail(lexed.status());
	}
	return parseTokens(lexed.value(), path);
}

} // namespace rt::scene
