/**
 * @file EventHandler.cpp
 * @author RT Team - Dev C (Renderer)  
 * @date 2026-04-01
 * @brief EventHandler implementation
 */

#include "EventHandler.hpp"
#include "Camera.hpp"
#include <SDL2/SDL.h>

EventHandler::EventHandler() : cameraControl(nullptr), cameraMoveSpeed(0.5), 
                               cameraRotateSpeed(0.05), windowCloseRequested(false),
                               windowResized(false), newWindowWidth(0), newWindowHeight(0) {}

EventHandler::~EventHandler()
{
    // TODO: Cleanup resources if needed
}

bool EventHandler::pollEvents()
{
    // TODO: Poll SDL events and dispatch callbacks
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // TODO: Handle different event types
        // SDL_QUIT, SDL_KEYDOWN, SDL_KEYUP, SDL_WINDOWEVENT
        if (event.type == SDL_QUIT) {
            windowCloseRequested = true;
            if (quitCallback) quitCallback();
            return true;
        }
    }
    processKeyboardInput();
    return false;
}

void EventHandler::onQuit(QuitCallback callback) { quitCallback = callback; }
void EventHandler::onKeyPress(KeyPressCallback callback) { keyPressCallback = callback; }
void EventHandler::onKeyRelease(KeyReleaseCallback callback) { keyReleaseCallback = callback; }
void EventHandler::onExpose(ExposeCallback callback) { exposeCallback = callback; }

void EventHandler::setupCameraControls(Camera* camera, double moveSpeed, double rotateSpeed)
{
    // TODO: Setup camera control
    cameraControl = camera;
    cameraMoveSpeed = moveSpeed;
    cameraRotateSpeed = rotateSpeed;
}

bool EventHandler::isKeyPressed(int keyCode) const
{
    // TODO: Check if key is currently pressed
    return false;
}

bool EventHandler::isWindowCloseRequested() const { return windowCloseRequested; }
void EventHandler::resetWindowCloseFlag() { windowCloseRequested = false; }
bool EventHandler::wasWindowResized() const { return windowResized; }

bool EventHandler::getNewWindowSize(int& width, int& height) const
{
    // TODO: Return new window size if resized
    if (windowResized) {
        width = newWindowWidth;
        height = newWindowHeight;
        return true;
    }
    return false;
}

void EventHandler::processKeyboardInput()
{
    // TODO: Process continuous key input for camera control
}
