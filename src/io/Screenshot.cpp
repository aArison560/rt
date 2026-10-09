// Capture d'ecran (T077) — implementation.
// Voir `include/rt/io/Screenshot.hpp` pour le contrat. Horodatage via
// `std::time` + `strftime` (chemin froid, allocations autorisees).

#include "rt/io/Screenshot.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>

#include "rt/io/ImageWriter.hpp"
#include "rt/render/Framebuffer.hpp"

namespace rt::io {

std::string screenshotName(long long epochSeconds) {
	std::time_t when = static_cast<std::time_t>(epochSeconds);
	std::tm parts{};
#if defined(_WIN32)
	localtime_s(&parts, &when);
#else
	localtime_r(&when, &parts);
#endif
	char buffer[64] = {};
	std::strftime(buffer, sizeof(buffer), "screenshot_%Y%m%d_%H%M%S.png", &parts);
	return std::string(buffer);
}

Result<std::string> saveScreenshot(const render::Framebuffer& fb, const std::string& dir) {
	if (fb.width() <= 0 || fb.height() <= 0 || fb.displayData() == nullptr) {
		return Result<std::string>::fail(
		    Status::error(StatusCode::InvalidArgument, "empty framebuffer, nothing to save", __LINE__));
	}
	if (dir.empty() || dir.size() > 512) {
		return Result<std::string>::fail(
		    Status::error(StatusCode::InvalidArgument, "bad screenshot dir", __LINE__));
	}
	const long long now =
	    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
	        .count();
	const std::string path = dir + "/" + screenshotName(now);
	if (Status status = writeImage(fb, path); status.isError()) {
		return Result<std::string>::fail(status);
	}
	return Result<std::string>::ok(path);
}

} // namespace rt::io
