#pragma once
#include "core/Scene.hpp"
#include <cstdint>
int	sdl_init(Scene *scene);
int	sdl_draw(uint32_t *buffer, int w, int h);
void	sdl_events(int *running);
void	sdl_cleanup(void);
int	ui_init(int argc, char **argv, Scene *scene);
int	ui_update(Scene *scene);
