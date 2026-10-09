// Pool de threads (T063) — voir `include/rt/sched/ThreadPool.hpp`.
// Chemin froid uniquement (creation `jthread`, file `std::function`,
// verrous) ; les taches elles-memes (tuiles) sont `noexcept` et disjointes.
// Aucun `throw` explicite ici (R2) hors allocations du chemin froid qui
// remontent au filet de `main` comme `collectSceneObjects` en T046.

#include "rt/sched/ThreadPool.hpp"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rt::sched {

struct ThreadPool::Impl {
	std::vector<std::jthread> workers;
	std::queue<std::function<void()>> queue;
	std::mutex mutex;
	std::condition_variable cvTask;
	std::condition_variable cvIdle;
	std::size_t pending = 0;
	bool stop = false;
	bool errorFlag = false;
	std::string errorMessage;
};

// Boucle d'un travailleur : attend une tache (condition, pas de spin),
// l'execute **hors verrou**, intercepte toute levee (pas de `terminate`).
// Methode statique (accede au `Impl` prive) ; `noexcept` hors allocations
// du chemin froid (le `catch` ne relance jamais).
void ThreadPool::workerMain(ThreadPool::Impl* impl) {
	while (true) {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(impl->mutex);
			impl->cvTask.wait(lock, [impl] { return impl->stop || !impl->queue.empty(); });
			if (impl->stop && impl->queue.empty()) {
				return;
			}
			if (impl->queue.empty()) {
				continue;
			}
			task = std::move(impl->queue.front());
			impl->queue.pop();
		}
		try {
			task();
		} catch (const std::exception& e) {
			std::lock_guard<std::mutex> lock(impl->mutex);
			if (!impl->errorFlag) {
				impl->errorFlag = true;
				impl->errorMessage = e.what();
			}
		} catch (...) {
			std::lock_guard<std::mutex> lock(impl->mutex);
			if (!impl->errorFlag) {
				impl->errorFlag = true;
				impl->errorMessage = "unknown task exception";
			}
		}
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			if (impl->pending > 0) {
				--impl->pending;
			}
			if (impl->pending == 0) {
				impl->cvIdle.notify_all();
			}
		}
	}
}

ThreadPool::ThreadPool(std::size_t numThreads) {
	if (numThreads == 0) {
		numThreads = 1;
	} else if (numThreads > 256) {
		numThreads = 256;
	}
	workerCount_ = numThreads;
	impl_ = std::make_unique<Impl>();
	for (std::size_t i = 0; i < workerCount_; ++i) {
		impl_->workers.emplace_back([worker = impl_.get()] { workerMain(worker); });
	}
}

ThreadPool::~ThreadPool() {
	if (impl_ == nullptr) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->stop = true;
	}
	impl_->cvTask.notify_all();
	// `jthread` joint automatiquement (arret propre, draine la file : les
	// travailleurs sortent quand `stop && queue.empty()`).
}

void ThreadPool::submit(std::function<void()> task) {
	if (impl_ == nullptr) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->queue.push(std::move(task));
		++impl_->pending;
	}
	impl_->cvTask.notify_one();
}

void ThreadPool::waitIdle() {
	if (impl_ == nullptr) {
		return;
	}
	std::unique_lock<std::mutex> lock(impl_->mutex);
	impl_->cvIdle.wait(lock, [this] { return impl_->pending == 0; });
}

bool ThreadPool::hasError() const {
	if (impl_ == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->errorFlag;
}

std::string ThreadPool::firstError() const {
	if (impl_ == nullptr) {
		return {};
	}
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->errorMessage;
}

} // namespace rt::sched
