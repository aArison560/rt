/**
 * @file Window.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief SDL2 window and rendering context
 *
 * Manages SDL2 window creation, rendering surface, and display updates.
 * Provides methods to initialize window, push pixels, and handle display.
 *
 * @relationships
 * - Uses: ImageBuffer (pixel data source)
 * - Uses: SDL2 library
 * - Used by: main, EventHandler
 * - Manages: SDL_Window, SDL_Renderer, SDL_Texture
 */

#pragma once

#include "ImageBuffer.hpp"
#include <string>
#include <memory>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class Window
{
public:
    /**
     * @brief Default constructor - creates uninitialized window
     */
    Window();

    /**
     * @brief Constructor with initial parameters
     * @param title Window title
     * @param width Window width in pixels
     * @param height Window height in pixels
     */
    Window(const std::string& title, int width, int height);

    /**
     * @brief Destructor - cleans up SDL resources
     */
    ~Window();

    /**
     * @brief Copy is deleted (window resources cannot be shared)
     */
    Window(const Window&) = delete;

    /**
     * @brief Copy assignment is deleted
     */
    Window& operator=(const Window&) = delete;

    /**
     * @brief Initialize window with SDL2
     * @param title Window title
     * @param width Window width
     * @param height Window height
     * @return true if initialization successful
     */
    bool initialize(const std::string& title, int width, int height);

    /**
     * @brief Check if window is initialized
     * @return true if window is valid and ready
     */
    bool isInitialized() const;

    /**
     * @brief Cleanup and close window
     */
    void close();

    /**
     * @brief Get window width
     * @return Width in pixels
     */
    int getWidth() const;

    /**
     * @brief Get window height
     * @return Height in pixels
     */
    int getHeight() const;

    /**
     * @brief Get window title
     * @return Current title
     */
    const std::string& getTitle() const;

    /**
     * @brief Set window title
     * @param title New window title
     */
    void setTitle(const std::string& title);

    /**
     * @brief Update display from image buffer
     * @param buffer The ImageBuffer to display
     * @return true if update successful
     */
    bool updateDisplay(const ImageBuffer& buffer);

    /**
     * @brief Update display from raw pixel data
     * @param pixelData Pixel data in RGBA format (width*height*4 bytes)
     * @param dataSize Size of pixel data in bytes
     * @return true if update successful
     */
    bool updateDisplay(const unsigned char* pixelData, size_t dataSize);

    /**
     * @brief Present current frame to display
     */
    void present();

    /**
     * @brief Clear window to black
     */
    void clear();

    /**
     * @brief Swap buffers / update display
     */
    void swapBuffers();

    /**
     * @brief Set vsync (vertical sync)
     * @param enabled Whether to enable vsync
     * @return true if successful
     */
    bool setVSync(bool enabled);

    /**
     * @brief Get SDL window pointer (for advanced use)
     * @return SDL_Window* (nullptr if not initialized)
     */
    SDL_Window* getSDLWindow() const;

    /**
     * @brief Get SDL renderer pointer (for advanced use)
     * @return SDL_Renderer* (nullptr if not initialized)
     */
    SDL_Renderer* getSDLRenderer() const;

private:
    std::string title;        ///< Window title
    int width;                ///< Window width
    int height;               ///< Window height
    SDL_Window* sdlWindow;    ///< SDL window pointer
    SDL_Renderer* sdlRenderer; ///< SDL renderer pointer
    SDL_Texture* sdlTexture;  ///< SDL texture for display
    bool initialized;         ///< Whether window is initialized

    /**
     * @brief Create or recreate texture
     * @return true if texture creation successful
     */
    bool createTexture();

    /**
     * @brief Destroy texture
     */
    void destroyTexture();
};
