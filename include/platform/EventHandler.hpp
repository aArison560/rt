/**
 * @file EventHandler.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief SDL2 event handling system
 *
 * Processes SDL2 events including window events, keyboard input, and mouse input.
 * Provides callbacks for application logic and camera control.
 *
 * @relationships
 * - Uses: SDL2 library, Camera (for input control)
 * - Used by: main (event loop)
 * - Interfaces with: Window, Renderer, Camera
 */

#pragma once

#include <functional>
#include <string>

class Camera;

/**
 * @brief Event callback types
 */
using QuitCallback = std::function<void()>;
using KeyPressCallback = std::function<void(int keyCode)>;
using KeyReleaseCallback = std::function<void(int keyCode)>;
using ExposeCallback = std::function<void()>;

class EventHandler
{
public:
    /**
     * @brief Default constructor
     */
    EventHandler();

    /**
     * @brief Destructor
     */
    ~EventHandler();

    /**
     * @brief Process all pending SDL events
     * @return true if quit event received
     */
    bool pollEvents();

    /**
     * @brief Register quit event callback
     * @param callback Function to call on quit event
     */
    void onQuit(QuitCallback callback);

    /**
     * @brief Register key press callback
     * @param callback Function to call on key press
     */
    void onKeyPress(KeyPressCallback callback);

    /**
     * @brief Register key release callback
     * @param callback Function to call on key release
     */
    void onKeyRelease(KeyReleaseCallback callback);

    /**
     * @brief Register window expose/redraw callback
     * @param callback Function to call on expose event
     */
    void onExpose(ExposeCallback callback);

    /**
     * @brief Setup camera keyboard controls
     * @param camera Pointer to camera to control
     * @param moveSpeed Speed of camera movement (units per frame)
     * @param rotateSpeed Speed of camera rotation (radians per frame)
     */
    void setupCameraControls(Camera* camera, double moveSpeed = 0.5, double rotateSpeed = 0.05);

    /**
     * @brief Get current keyboard state
     * Useful for continuous key checking
     * @param keyCode SDL keycode to check
     * @return true if key is currently pressed
     */
    bool isKeyPressed(int keyCode) const;

    /**
     * @brief Check if window close requested
     * @return true if close was requested
     */
    bool isWindowCloseRequested() const;

    /**
     * @brief Reset window close flag
     */
    void resetWindowCloseFlag();

    /**
     * @brief Check if window resized
     * @return true if window was resized
     */
    bool wasWindowResized() const;

    /**
     * @brief Get new window dimensions after resize
     * @param width Output: new width
     * @param height Output: new height
     * @return true if window was resized
     */
    bool getNewWindowSize(int& width, int& height) const;

private:
    QuitCallback quitCallback;           ///< Quit event callback
    KeyPressCallback keyPressCallback;    ///< Key press callback
    KeyReleaseCallback keyReleaseCallback; ///< Key release callback
    ExposeCallback exposeCallback;        ///< Expose event callback
    Camera* cameraControl;               ///< Camera for keyboard control (optional)
    double cameraMoveSpeed;              ///< Camera movement speed
    double cameraRotateSpeed;            ///< Camera rotation speed
    bool windowCloseRequested;           ///< Whether close was requested
    bool windowResized;                  ///< Whether window was resized
    int newWindowWidth;                  ///< New window width after resize
    int newWindowHeight;                 ///< New window height after resize

    /**
     * @brief Process keyboard input for continuous movement
     */
    void processKeyboardInput();
};
