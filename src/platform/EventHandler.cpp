/**
 * @file EventHandler.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief EventHandler implementation
 */

#include "platform/EventHandler.hpp"
#include "scene/Camera.hpp"
#include <SDL2/SDL.h>

EventHandler::EventHandler()
    : cameraControl(nullptr)
    , cameraMoveSpeed(0.5)
    , cameraRotateSpeed(0.05)
    , windowCloseRequested(false)
    , windowResized(false)
    , newWindowWidth(0)
    , newWindowHeight(0)
    , cameraControlEnabled(true)
{
}

EventHandler::~EventHandler()
{
}

bool EventHandler::pollEvents()
{
    windowResized = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Forward event to ImGui callback first (before internal processing)
        if (sdlEventCallback) {
            sdlEventCallback(event);
        }

        switch (event.type) {
            case SDL_QUIT:
                windowCloseRequested = true;
                if (quitCallback) {
                    quitCallback();
                }
                return true;
            case SDL_KEYDOWN:
                if (keyPressCallback) {
                    keyPressCallback(event.key.keysym.sym);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (mouseClickCallback) {
                    mouseClickCallback(event.button.x, event.button.y, event.button.button);
                }
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    windowCloseRequested = true;
                    if (quitCallback) {
                        quitCallback();
                    }
                    return true;
                }
                break;
            case SDL_KEYUP:
                if (keyReleaseCallback) {
                    keyReleaseCallback(event.key.keysym.sym);
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    if (exposeCallback) {
                        exposeCallback();
                    }
                } else if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                           event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    windowResized = true;
                    newWindowWidth = event.window.data1;
                    newWindowHeight = event.window.data2;
                }
                break;
            default:
                break;
        }
    }
    processKeyboardInput();
    return false;
}

void EventHandler::onQuit(QuitCallback callback) { quitCallback = callback; }
void EventHandler::onKeyPress(KeyPressCallback callback) { keyPressCallback = callback; }
void EventHandler::onKeyRelease(KeyReleaseCallback callback) { keyReleaseCallback = callback; }
void EventHandler::onExpose(ExposeCallback callback) { exposeCallback = callback; }
void EventHandler::onSDLEvent(SDLEventCallback callback) { sdlEventCallback = callback; }
void EventHandler::onMouseClick(MouseClickCallback callback) { mouseClickCallback = callback; }

void EventHandler::setupCameraControls(Camera* camera, double moveSpeed, double rotateSpeed)
{
    cameraControl = camera;
    cameraMoveSpeed = moveSpeed;
    cameraRotateSpeed = rotateSpeed;
}

bool EventHandler::isKeyPressed(int keyCode) const
{
    int keyCount = 0;
    const Uint8* state = SDL_GetKeyboardState(&keyCount);
    if (!state || keyCount <= 0) {
        return false;
    }
    SDL_Scancode scan = SDL_GetScancodeFromKey(keyCode);
    if (scan == SDL_SCANCODE_UNKNOWN || static_cast<int>(scan) >= keyCount) {
        return false;
    }
    return state[scan] != 0;
}

bool EventHandler::isWindowCloseRequested() const { return windowCloseRequested; }
void EventHandler::resetWindowCloseFlag() { windowCloseRequested = false; }
bool EventHandler::wasWindowResized() const { return windowResized; }

bool EventHandler::getNewWindowSize(int& width, int& height) const
{
    if (windowResized) {
        width = newWindowWidth;
        height = newWindowHeight;
        return true;
    }
    return false;
}

void EventHandler::setCameraControlEnabled(bool enabled)
{
    cameraControlEnabled = enabled;
}

bool EventHandler::getCameraControlEnabled() const
{
    return cameraControlEnabled;
}

void EventHandler::processKeyboardInput()
{
    if (!cameraControl || !cameraControlEnabled) {
        return;
    }

    if (isKeyPressed(SDLK_w)) cameraControl->moveForward(cameraMoveSpeed);
    if (isKeyPressed(SDLK_s)) cameraControl->moveBackward(cameraMoveSpeed);
    if (isKeyPressed(SDLK_a)) cameraControl->moveLeft(cameraMoveSpeed);
    if (isKeyPressed(SDLK_d)) cameraControl->moveRight(cameraMoveSpeed);
    if (isKeyPressed(SDLK_q)) cameraControl->moveDown(cameraMoveSpeed);
    if (isKeyPressed(SDLK_e)) cameraControl->moveUp(cameraMoveSpeed);

    if (isKeyPressed(SDLK_UP)) cameraControl->rotatePitch(-cameraRotateSpeed);
    if (isKeyPressed(SDLK_DOWN)) cameraControl->rotatePitch(cameraRotateSpeed);
    if (isKeyPressed(SDLK_LEFT)) cameraControl->rotateYaw(-cameraRotateSpeed);
    if (isKeyPressed(SDLK_RIGHT)) cameraControl->rotateYaw(cameraRotateSpeed);
}
