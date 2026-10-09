#pragma once

// Pool de threads reutilisable (T063) — un backend d'execution parmi
// d'autres (cf. `docs/INSPIRATION_BLENDER.md` §3.1 : *device abstraction*).
// Le pool est cree **une fois** par rendu (`std::jthread`, cout de creation
// paye une fois), puis nourri par tuiles de 32×32 (`renderRegion()` en
// `src/render/Renderer.cpp`). Arret propre (`stop` + `join` via `jthread`),
// boucle d'attente econome (variables de condition, pas de spin actif).
// Exceptions interceptees **par tache** : une tache qui leve pose
// `hasError()` (message via `firstError()`) au lieu de `std::terminate`
// (filet `main` intact, R2). Le hot path par pixel ne passe jamais ici
// (R3 : `submit`/`waitIdle` = chemin froid, allocations `std::function`
// autorisees — une par tuile, pas par pixel).
// Thread-safety : `submit()`/`waitIdle()`/`hasError()` verrouilles ;
// les taches elles-memes doivent etre disjointes (framebuffer par tuiles
// disjointes, contexte de trace lecture seule) pour rester TSan-vert.

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace rt::sched {

class ThreadPool {
public:
	// `numThreads` 1..256 (borne CLI `--threads`, R1) ; 0 -> 1, > 256 -> 256.
	explicit ThreadPool(std::size_t numThreads);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	~ThreadPool();

	// Enfile une tache (copiee en file, execution par un travailleur).
	// La tache ne doit ni allouer dans sa boucle chaude ni lever (si elle
	// leve, l'erreur est capturee et `hasError()` passe a vrai).
	void submit(std::function<void()> task);

	// Bloque jusqu'a ce que toutes les taches soumises soient terminees
	// (attente sur condition, pas de spin). Reutilisable entre batches.
	void waitIdle();

	[[nodiscard]] std::size_t size() const noexcept { return workerCount_; }
	[[nodiscard]] bool hasError() const;
	[[nodiscard]] std::string firstError() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
	std::size_t workerCount_ = 1;

	static void workerMain(Impl* impl);
};

} // namespace rt::sched
