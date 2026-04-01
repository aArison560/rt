/**
 * @file Window.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Window implementation
 */

#include "platform/Window.hpp"
#include <SDL2/SDL.h>

Window::Window() : width(0), height(0), sdlWindow(nullptr), sdlRenderer(nullptr),
                   sdlTexture(nullptr), initialized(false) {}

Window::Window(const std::string& title, int width, int height)
    : title(title), width(0), height(0), sdlWindow(nullptr), sdlRenderer(nullptr),
      sdlTexture(nullptr), initialized(false)
{
    initialize(title, width, height);
}

Window::~Window()
{
    close();
}

bool Window::initialize(const std::string& title, int width, int height)
{
    // TODO: Initialize SDL window and renderer
    // SDL_CreateWindow, SDL_CreateRenderer, create texture
    return false;
}

bool Window::isInitialized() const
{
    return initialized;
}

void Window::close()
{
    // TODO: Cleanup SDL resources
    destroyTexture();
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (sdlWindow) SDL_DestroyWindow(sdlWindow);
    initialized = false;
}

int Window::getWidth() const { return width; }
int Window::getHeight() const { return height; }
const std::string& Window::getTitle() const { return title; }

void Window::setTitle(const std::string& title)
{
    // TODO: Set window title
    this->title = title;
    if (sdlWindow) SDL_SetWindowTitle(sdlWindow, title.c_str());
}

bool Window::updateDisplay(const ImageBuffer& buffer)
{
    // TODO: Update texture from image buffer
    return false;
}

bool Window::updateDisplay(const unsigned char* pixelData, size_t dataSize)
{
    // TODO: Update texture from raw pixel data
    return false;
}

void Window::present()
{
    // TODO: Present renderer
    if (sdlRenderer) SDL_RenderPresent(sdlRenderer);
}

void Window::clear()
{
    // TODO: Clear to black
    if (sdlRenderer) SDL_RenderClear(sdlRenderer);
}

void Window::swapBuffers()
{
    present();
}

bool Window::setVSync(bool enabled)
{
    // TODO: Set vertical sync
    return true;
}

SDL_Window* Window::getSDLWindow() const { return sdlWindow; }
SDL_Renderer* Window::getSDLRenderer() const { return sdlRenderer; }

bool Window::createTexture()
{
    // TODO: Create SDL_Texture for display
    return false;
}

void Window::destroyTexture()
{
    // TODO: Destroy texture
    if (sdlTexture) {
        SDL_DestroyTexture(sdlTexture);
        sdlTexture = nullptr;
    }
}
