// Perturbation de normale par onde (T107, *Disruptions* 1), Catch2.
// `sine` : la normale differe periodiquement, `amplitude = 0` = rendu
// identique au sans-pattern.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/shading/Pattern.hpp"

TEST_CASE("wave : normale ondulee periodique, amplitude 0 = off (T107)", "[pattern][t107]") {
	const rt::Vec3 up(0.0F, 1.0F, 0.0F);
	const rt::Vec3 at(1.0F, 2.0F, 3.0F);
	// Amplitude 0 -> normale inchangee (bit-identique).
	const rt::Vec3 off = rt::shading::waveNormal(up, at, 0.0F, 3.0F);
	REQUIRE(off.x == Catch::Approx(up.x));
	REQUIRE(off.y == Catch::Approx(up.y));
	REQUIRE(off.z == Catch::Approx(up.z));
	// Amplitude > 0 -> differe (sauf cas pathologique du zero du sinus).
	const rt::Vec3 on = rt::shading::waveNormal(up, at, 0.4F, 3.0F);
	const double diff = std::fabs(static_cast<double>(on.x - up.x)) +
	                    std::fabs(static_cast<double>(on.y - up.y)) +
	                    std::fabs(static_cast<double>(on.z - up.z));
	REQUIRE(diff > 1e-4);
	// Unitaire apres perturbation.
	const double len = std::sqrt(static_cast<double>(on.x * on.x + on.y * on.y + on.z * on.z));
	REQUIRE(len == Catch::Approx(1.0).margin(1e-5));
	// Periodicite : +2*pi/f sur les 3 axes -> meme perturbation.
	const float freq = 3.0F;
	const float period = 2.0F * 3.14159265358979323846F / freq;
	const rt::Vec3 shifted(at.x + period, at.y + period, at.z + period);
	const rt::Vec3 onShifted = rt::shading::waveNormal(up, shifted, 0.4F, freq);
	REQUIRE(onShifted.x == Catch::Approx(on.x).margin(1e-5));
	REQUIRE(onShifted.y == Catch::Approx(on.y).margin(1e-5));
	REQUIRE(onShifted.z == Catch::Approx(on.z).margin(1e-5));
}

TEST_CASE("wave : amplitude 0 identique au sans-pattern (T107)", "[pattern][t107]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_wave.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	rt::render::Framebuffer fbWave;
	rt::render::RenderParams params{.width = 80, .height = 60, .spp = 2, .maxDepth = 2, .seed = 15};
	REQUIRE(rt::render::render(result.value(), fbWave, params, nullptr).isOk());
	// Meme scene, motifs retires : avec `scale = 0` l'onde est off, donc
	// les deux rendus doivent etre proches ; avec motifs presents ils
	// different du sans-motif (l'onde a un effet visible).
	rt::scene::Scene plain = result.value();
	for (auto& obj : plain.objects) {
		obj.material.pattern.present = false;
	}
	rt::render::Framebuffer fbPlain;
	REQUIRE(rt::render::render(plain, fbPlain, params, nullptr).isOk());
	int different = 0;
	const int total = fbWave.width() * fbWave.height();
	for (int y = 0; y < fbWave.height(); ++y) {
		for (int x = 0; x < fbWave.width(); ++x) {
			const rt::Vec3 a = fbWave.accumAt(x, y);
			const rt::Vec3 b = fbPlain.accumAt(x, y);
			const int na = fbWave.samplesAt(x, y);
			const int nb = fbPlain.samplesAt(x, y);
			const double d = std::fabs(static_cast<double>(a.x) / static_cast<double>(na) -
			                           static_cast<double>(b.x) / static_cast<double>(nb)) +
			                 std::fabs(static_cast<double>(a.y) / static_cast<double>(na) -
			                           static_cast<double>(b.y) / static_cast<double>(nb)) +
			                 std::fabs(static_cast<double>(a.z) / static_cast<double>(na) -
			                           static_cast<double>(b.z) / static_cast<double>(nb));
			if (d > 0.03) {
				++different;
			}
		}
	}
	REQUIRE(different > total / 100);
	// Amplitude 0 = sans effet : octet par octet identique au sans-pattern.
	rt::scene::Scene zero = result.value();
	for (auto& obj : zero.objects) {
		obj.material.pattern.scale = 0.0F;
	}
	rt::render::Framebuffer fbZero;
	REQUIRE(rt::render::render(zero, fbZero, params, nullptr).isOk());
	for (int y = 0; y < fbZero.height(); ++y) {
		for (int x = 0; x < fbZero.width(); ++x) {
			const rt::Vec3 a = fbZero.accumAt(x, y);
			const rt::Vec3 b = fbPlain.accumAt(x, y);
			REQUIRE(a.x == Catch::Approx(b.x));
			REQUIRE(a.y == Catch::Approx(b.y));
			REQUIRE(a.z == Catch::Approx(b.z));
		}
	}
}
