// Ligne de commande (T026) — parsing sans acces fichier ni SDL (R1/R2).
// Voir `include/rt/app/Options.hpp` pour le contrat. Tout passe par
// `std::from_chars` (aucune levee, filet `main` intact) ; chaque echec
// renvoie `InvalidArgument` avec un message actionnable.

#include "rt/app/Options.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace rt::app {

namespace {

constexpr int kMinDim = 1;
constexpr int kMaxDim = 8192;
constexpr int kMinSpp = 1;
constexpr int kMaxSpp = 1024;
constexpr long long kMinSeed = 0;
constexpr long long kMaxSeed = 4294967295LL;
constexpr int kMinThreads = 1;
constexpr int kMaxThreads = 256;
constexpr int kMinTiles = 1;
constexpr int kMaxTiles = 64;

[[nodiscard]] Status fail(std::string_view detail, int line) {
	return Status::error(StatusCode::InvalidArgument, detail, line);
}

// Entier strict : tout le texte consomme, borne verifiee.
[[nodiscard]] bool parseIntStrict(std::string_view text, long long& out) {
	if (text.empty()) {
		return false;
	}
	const char* first = text.data();
	const char* last = text.data() + text.size();
	long long value = 0;
	const std::from_chars_result res = std::from_chars(first, last, value);
	if (res.ec != std::errc() || res.ptr != last) {
		return false;
	}
	out = value;
	return true;
}

[[nodiscard]] Status checkDim(long long value, std::string_view what) {
	if (value < kMinDim || value > kMaxDim) {
		std::string msg("bad ");
		msg.append(what);
		msg.append(": expected 1..8192");
		return fail(msg, __LINE__);
	}
	return Status::ok();
}

[[nodiscard]] Status parseTileValue(std::string_view text, int& kOut, int& nOut) {
	const std::string_view::size_type slash = text.find('/');
	if (slash == std::string_view::npos) {
		return fail("bad --tile: expected k/n (e.g. 0/4)", __LINE__);
	}
	const std::string_view left = text.substr(0, slash);
	const std::string_view right = text.substr(slash + 1);
	long long k = 0;
	long long n = 0;
	if (!parseIntStrict(left, k) || !parseIntStrict(right, n)) {
		return fail("bad --tile: expected k/n with integers (e.g. 0/4)", __LINE__);
	}
	if (n < kMinTiles || n > kMaxTiles) {
		return fail("bad --tile: n expected 1..64", __LINE__);
	}
	if (k < 0 || k >= n) {
		return fail("bad --tile: k expected 0..n-1", __LINE__);
	}
	kOut = static_cast<int>(k);
	nOut = static_cast<int>(n);
	return Status::ok();
}

struct Cursor {
	const std::vector<std::string_view>* args = nullptr;
	std::size_t pos = 1;
	bool endOfOptions = false;
};

[[nodiscard]] bool hasMore(const Cursor& cur) {
	return cur.pos < cur.args->size();
}

[[nodiscard]] std::string_view peek(const Cursor& cur) {
	return (*cur.args)[cur.pos];
}

[[nodiscard]] Status needValue(const Cursor& cur, std::string_view opt, std::string_view& out) {
	if (cur.pos + 1 >= cur.args->size()) {
		std::string msg("missing value for '");
		msg.append(opt);
		msg.append("'");
		return fail(msg, __LINE__);
	}
	out = (*cur.args)[cur.pos + 1];
	return Status::ok();
}

} // namespace

std::string usageText() {
	std::string out;
	out += "usage: rt <scene.rt> [width height] [options]\n";
	out += "\n";
	out += "  <scene.rt>     scene file (.rt) (required unless --help/--version)\n";
	out += "  [width height] override resolution (1..8192 each, both or none)\n";
	out += "\n";
	out += "options:\n";
	out += "  --out <file>, --out=<file>   output image path\n";
	out += "  --spp <n>, --spp=<n>         samples per pixel (1..1024)\n";
	out += "  --seed <n>, --seed=<n>       rng seed (0..4294967295)\n";
	out += "  --threads <n>                render threads (1..256)\n";
	out += "  --tile <k/n>                 tile k of n, 0 pixel overlap (n 1..64, k 0..n-1)\n";
	out += "  --width <n>, --height <n>    aliases for positional width/height (1..8192)\n";
	out += "  --headless                   no window (default when --out is given)\n";
	out += "  --quiet, -q                  suppress progress output\n";
	out += "  --help, -h                   show this help (exit 0)\n";
	out += "  --version, -v                show version (exit 0)\n";
	out += "  --                         end of options\n";
	out += "\n";
	out += "exit codes: 0 ok/help/version, 1 scene error, 2 cli usage error.\n";
	out += "examples:\n";
	out += "  rt scenes/m.rt 640 480 --out /tmp/a.png\n";
	out += "  rt scenes/m.rt --width 640 --height 480 --spp 16 --seed 42\n";
	out += "  rt scenes/m.rt --tile 1/4 --spp 16 --seed 42 --out /tmp/t1.png\n";
	return out;
}

namespace {

[[nodiscard]] Result<Options> parseViews(const std::vector<std::string_view>& args) {
	Options opts;
	if (args.empty()) {
		return Result<Options>::fail(fail("no scene file (try --help)", __LINE__));
	}
	Cursor cur;
	cur.args = &args;
	cur.pos = 1;
	cur.endOfOptions = false;

	int positionalCount = 0;
	bool seenOut = false;
	bool seenSpp = false;
	bool seenSeed = false;
	bool seenThreads = false;
	bool seenTile = false;
	bool seenWidthOpt = false;
	bool seenHeightOpt = false;

	while (hasMore(cur)) {
		const std::string_view arg = peek(cur);
		const bool isDash = !cur.endOfOptions && arg.size() > 0 && arg[0] == '-';
		if (!isDash) {
			// Positionnel : scene, puis width, puis height.
			if (positionalCount == 0) {
				opts.scenePath.assign(arg.data(), arg.size());
				opts.hasScene = true;
				positionalCount = 1;
				++cur.pos;
				continue;
			}
			if (positionalCount == 1 || positionalCount == 2) {
				long long value = 0;
				if (!parseIntStrict(arg, value)) {
					std::string msg("bad width/height: '");
					msg.append(arg);
					msg.append("' is not an integer 1..8192");
					return Result<Options>::fail(fail(msg, __LINE__));
				}
				if (Status st = checkDim(value, positionalCount == 1 ? "width" : "height");
				    st.isError()) {
					return Result<Options>::fail(st);
				}
				if (positionalCount == 1) {
					opts.width = static_cast<int>(value);
					opts.hasWidth = true;
				} else {
					opts.height = static_cast<int>(value);
					opts.hasHeight = true;
				}
				positionalCount++;
				++cur.pos;
				continue;
			}
			std::string msg("too many positional arguments: '");
			msg.append(arg);
			msg.append("'");
			return Result<Options>::fail(fail(msg, __LINE__));
		}
		// Options (apres `--`, tout est positionnel : traite plus haut).
		if (arg == "--") {
			cur.endOfOptions = true;
			++cur.pos;
			continue;
		}
		if (arg == "--help" || arg == "-h") {
			opts.showHelp = true;
			++cur.pos;
			continue;
		}
		if (arg == "--version" || arg == "-v") {
			opts.showVersion = true;
			++cur.pos;
			continue;
		}
		if (arg == "--headless") {
			opts.headless = true;
			++cur.pos;
			continue;
		}
		if (arg == "--quiet" || arg == "-q") {
			opts.quiet = true;
			++cur.pos;
			continue;
		}
		// Valeurs `--opt value` et `--opt=value`.
		std::string_view opt = arg;
		std::string_view inlineValue;
		const std::string_view::size_type eq = arg.find('=');
		if (eq != std::string_view::npos) {
			opt = arg.substr(0, eq);
			inlineValue = arg.substr(eq + 1);
		}
		if (opt == "--out" || opt == "-o") {
			if (seenOut) {
				return Result<Options>::fail(fail("duplicate --out", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--out", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			if (value.empty()) {
				return Result<Options>::fail(fail("bad --out: empty path", __LINE__));
			}
			if (value.size() > 1024) {
				return Result<Options>::fail(fail("bad --out: path too long", __LINE__));
			}
			opts.outPath.assign(value.data(), value.size());
			opts.hasOut = true;
			seenOut = true;
			++cur.pos;
			continue;
		}
		if (opt == "--spp") {
			if (seenSpp) {
				return Result<Options>::fail(fail("duplicate --spp", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--spp", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			long long n = 0;
			if (!parseIntStrict(value, n) || n < kMinSpp || n > kMaxSpp) {
				return Result<Options>::fail(fail("bad --spp: expected 1..1024", __LINE__));
			}
			opts.spp = static_cast<int>(n);
			opts.hasSpp = true;
			seenSpp = true;
			++cur.pos;
			continue;
		}
		if (opt == "--seed") {
			if (seenSeed) {
				return Result<Options>::fail(fail("duplicate --seed", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--seed", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			long long n = 0;
			if (!parseIntStrict(value, n) || n < kMinSeed || n > kMaxSeed) {
				return Result<Options>::fail(
				    fail("bad --seed: expected 0..4294967295", __LINE__));
			}
			opts.seed = n;
			opts.hasSeed = true;
			seenSeed = true;
			++cur.pos;
			continue;
		}
		if (opt == "--threads") {
			if (seenThreads) {
				return Result<Options>::fail(fail("duplicate --threads", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--threads", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			long long n = 0;
			if (!parseIntStrict(value, n) || n < kMinThreads || n > kMaxThreads) {
				return Result<Options>::fail(
				    fail("bad --threads: expected 1..256", __LINE__));
			}
			opts.threads = static_cast<int>(n);
			opts.hasThreads = true;
			seenThreads = true;
			++cur.pos;
			continue;
		}
		if (opt == "--tile") {
			if (seenTile) {
				return Result<Options>::fail(fail("duplicate --tile", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--tile", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			int k = 0;
			int n = 1;
			if (Status st = parseTileValue(value, k, n); st.isError()) {
				return Result<Options>::fail(st);
			}
			opts.tileIndex = k;
			opts.tileCount = n;
			opts.hasTile = true;
			seenTile = true;
			++cur.pos;
			continue;
		}
		if (opt == "--width") {
			if (seenWidthOpt) {
				return Result<Options>::fail(fail("duplicate --width", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--width", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			long long n = 0;
			if (!parseIntStrict(value, n)) {
				return Result<Options>::fail(
				    fail("bad --width: expected 1..8192", __LINE__));
			}
			if (Status st = checkDim(n, "width"); st.isError()) {
				return Result<Options>::fail(st);
			}
			opts.width = static_cast<int>(n);
			opts.hasWidth = true;
			seenWidthOpt = true;
			++cur.pos;
			continue;
		}
		if (opt == "--height") {
			if (seenHeightOpt) {
				return Result<Options>::fail(fail("duplicate --height", __LINE__));
			}
			std::string_view value = inlineValue;
			bool hasInline = (eq != std::string_view::npos);
			if (!hasInline) {
				if (Status st = needValue(cur, "--height", value); st.isError()) {
					return Result<Options>::fail(st);
				}
				++cur.pos;
			}
			long long n = 0;
			if (!parseIntStrict(value, n)) {
				return Result<Options>::fail(
				    fail("bad --height: expected 1..8192", __LINE__));
			}
			if (Status st = checkDim(n, "height"); st.isError()) {
				return Result<Options>::fail(st);
			}
			opts.height = static_cast<int>(n);
			opts.hasHeight = true;
			seenHeightOpt = true;
			++cur.pos;
			continue;
		}
		std::string msg("unknown option '");
		msg.append(arg);
		msg.append("' (try --help)");
		return Result<Options>::fail(fail(msg, __LINE__));
	}

	if (opts.showHelp || opts.showVersion) {
		return Result<Options>::ok(std::move(opts));
	}
	if (!opts.hasScene) {
		return Result<Options>::fail(fail("no scene file (try --help)", __LINE__));
	}
	if (positionalCount == 2) {
		return Result<Options>::fail(
		    fail("width and height must be given together", __LINE__));
	}
	if ((seenWidthOpt || seenHeightOpt) && positionalCount > 1) {
		return Result<Options>::fail(
		    fail("duplicate width/height: positional and --width/--height", __LINE__));
	}
	return Result<Options>::ok(std::move(opts));
}

} // namespace

Result<Options> parseOptions(int argc, char const* const* argv) {
	if (argc <= 0 || argv == nullptr) {
		return Result<Options>::fail(fail("no scene file (try --help)", __LINE__));
	}
	std::vector<std::string_view> args;
	args.reserve(static_cast<std::size_t>(argc));
	for (int i = 0; i < argc; ++i) {
		const char* item = argv[i];
		if (item == nullptr) {
			item = "";
		}
		args.emplace_back(item);
	}
	return parseViews(args);
}

Result<Options> parseOptionsVec(const std::vector<std::string>& args) {
	std::vector<std::string_view> views;
	views.reserve(args.size());
	for (const std::string& item : args) {
		views.emplace_back(item);
	}
	return parseViews(views);
}

} // namespace rt::app
