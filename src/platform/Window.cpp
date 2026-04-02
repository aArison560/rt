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

Window::Window(const std::string &title, int width, int height)
    : title(title), width(0), height(0), sdlWindow(nullptr), sdlRenderer(nullptr),
      sdlTexture(nullptr), initialized(false)
{
    initialize(title, width, height);
}

Window::~Window()
{
    close();
}

bool Window::initialize(const std::string &title, int width, int height)
{
    // TODO: Initialize SDL window and renderer
    // SDL_CreateWindow, SDL_CreateRenderer, create texture
    if (initialized)
        close();

    this->title = title;
    this->width = width;
    this->height = height;

    if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
            return false;
    }

    sdlWindow = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (!sdlWindow)
        return false;

    sdlRenderer = SDL_CreateRenderer(
        sdlWindow, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!sdlRenderer)
    {
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
        return false;
    }

    if (!createTexture())
    {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = nullptr;
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
        return false;
    }

    initialized = true;
    return true;
}

bool Window::isInitialized() const
{
    return initialized;
}

void Window::close()
{
    // TODO: Cleanup SDL resources
    destroyTexture();
    if (sdlRenderer)
        SDL_DestroyRenderer(sdlRenderer);
    if (sdlWindow)
        SDL_DestroyWindow(sdlWindow);
    initialized = false;
}

int Window::getWidth() const { return width; }
int Window::getHeight() const { return height; }
const std::string &Window::getTitle() const { return title; }

void Window::setTitle(const std::string &title)
{
    // TODO: Set window title
    this->title = title;
    if (sdlWindow)
        SDL_SetWindowTitle(sdlWindow, title.c_str());
}

bool Window::updateDisplay(const ImageBuffer &buffer)
{
    return updateDisplay(buffer.getDataConst(), buffer.getDataSize());
}

bool Window::updateDisplay(const unsigned char *pixelData, size_t dataSize)
{
    if (!initialized || !sdlRenderer || !pixelData || dataSize == 0)
        return false;

    const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (dataSize < expectedSize)
        return false;

    if (!sdlTexture && !createTexture())
        return false;

    if (SDL_UpdateTexture(sdlTexture, nullptr, pixelData, width * 4) != 0)
        return false;

    if (SDL_RenderClear(sdlRenderer) != 0)
        return false;

    if (SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr) != 0)
        return false;

    return true;
}

void Window::present()
{
    // TODO: Present renderer
    if (sdlRenderer)
        SDL_RenderPresent(sdlRenderer);
}

void Window::clear()
{
    // TODO: Clear to black
    if (sdlRenderer)
        SDL_RenderClear(sdlRenderer);
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

SDL_Window *Window::getSDLWindow() const { return sdlWindow; }
SDL_Renderer *Window::getSDLRenderer() const { return sdlRenderer; }

bool Window::createTexture()
{
    if (!sdlRenderer)
        return false;

    destroyTexture();

    sdlTexture = SDL_CreateTexture(
        sdlRenderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

    return sdlTexture != nullptr;
}

void Window::destroyTexture()
{
    // TODO: Destroy texture
    if (sdlTexture)
    {
        SDL_DestroyTexture(sdlTexture);
        sdlTexture = nullptr;
    }
}
