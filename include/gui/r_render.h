/**
 * @file r_render.h
 * @brief SDL2 renderer backend for microui
 *
 * Processes microui draw commands and renders them using SDL2's SDL_Renderer.
 */

#pragma once

#include <SDL2/SDL.h>

struct mu_Context;

#ifdef __cplusplus
extern "C" {
#endif

int r_init(mu_Context *ctx, SDL_Renderer *renderer);
void r_render(mu_Context *ctx, SDL_Renderer *renderer);

#ifdef __cplusplus
}
#endif
