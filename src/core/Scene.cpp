#include "core/Scene.hpp"

// Destructeur : libère objets C++ + liste C miroir.
Scene::~Scene(){ clear(); }

void	Scene::clear() {
	objects.clear(); // unique_ptr : pixels/shared_ptr libérés avec
	s_obj*	cur = objs; // liste C manuelle : delete un par un

	while (cur) {
		s_obj*	n = cur->next;
		delete cur;
		cur = n;
	}
	objs = nullptr;
}
