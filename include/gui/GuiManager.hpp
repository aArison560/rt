/**
 * @file GuiManager.hpp
 * @author RT Team - Dev C
 * @date 2026-07-05
 * @brief ImGui-based GUI overlay manager
 *
 * Manages the Dear ImGui lifecycle and renders interactive UI panels
 * over the ray traced image.
 *
 * @relationships
 * - Uses: Window (SDL2 window/renderer), Renderer (settings + ray-picking), EventHandler (events)
 * - Used by: main.cpp
 * - Integrates: Dear ImGui with SDL2 + SDL_Renderer backends
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include "platform/Window.hpp"
#include "platform/EventHandler.hpp"
#include "rendering/Renderer.hpp"
#include "scene/Scene.hpp"
#include "core/Vec3.hpp"

class GuiManager
{
public:
    /**
     * @brief Constructor
     * @param window Pointer to the SDL2 window
     * @param renderer Pointer to the ray tracer renderer
     * @param eventHandler Pointer to the event handler
     */
    GuiManager(Window* window, Renderer* renderer, EventHandler* eventHandler);

    /**
     * @brief Destructor
     */
    ~GuiManager();

    /**
     * @brief Initialize ImGui with SDL2 + SDL_Renderer backends
     * @return true if initialization succeeded
     */
    [[nodiscard]] bool initialize();

    /**
     * @brief Shutdown ImGui and clean up resources
     */
    void shutdown();

    /**
     * @brief Begin a new ImGui frame (call at start of each iteration)
     * Must be called BEFORE any SDL_Renderer operations in the frame.
     */
    void beginFrame();

    /**
     * @brief Render all GUI panels
     */
    void render();

    /**
     * @brief Render ImGui draw data on top of the existing SDL renderer output
     * Must be called AFTER window.updateDisplay() and BEFORE window.present().
     */
    void renderOverlay();

    /**
     * @brief Check if ImGui currently wants to capture keyboard input
     * @return true if ImGui sliders/text fields are active
     */
    [[nodiscard]] bool wantsCaptureKeyboard() const;

    /**
     * @brief Check if ImGui currently wants to capture mouse input
     * @return true if ImGui is interacting with mouse
     */
    [[nodiscard]] bool wantsCaptureMouse() const;

    /**
     * @brief Check if GUI is visible
     * @return true if panels are being drawn
     */
    [[nodiscard]] bool isVisible() const;

    /**
     * @brief Toggle GUI visibility
     */
    void toggleVisibility();

    /**
     * @brief Set the current scene pointer for editing
     * @param scene Pointer to the current scene (non-const for modifications)
     */
    void setScene(Scene* scene);

    /**
     * @brief Check if a re-render was requested via GUI
     * @return true if the scene needs to be re-rendered
     */
    [[nodiscard]] bool needsRedraw() const;

    /**
     * @brief Clear the re-render request flag
     */
    void clearRedrawFlag();

    /**
     * @brief Handle a mouse click for object picking
     * @param mouseX X screen coordinate
     * @param mouseY Y screen coordinate
     */
    void onMouseClick(int mouseX, int mouseY);

private:
    Window* window;           ///< SDL2 window
    Renderer* renderer;       ///< Ray tracer renderer
    EventHandler* eventHandler; ///< Event handler
    Scene* scene;             ///< Current scene (non-const for modifications)
    bool visible;             ///< Whether GUI panels are visible
    bool initialized;         ///< Whether ImGui is initialized
    bool redrawRequested;     ///< Whether a re-render is needed

    // FPS tracking
    uint64_t lastPerformanceCounter; ///< Last SDL performance counter value
    int frameCount;                 ///< Frame count for FPS averaging
    float fps;                      ///< Current FPS
    double frameTimeMs;             ///< Current frame time in ms

    // Object creation state
    bool showCreatePanel;           ///< Whether the creation panel is open
    int newObjectType;              ///< 0=Sphere, 1=Plane, 2=Cylinder, 3=Cone
    Vec3 newObjectPos;              ///< Position for the new object
    double newObjectRadius;         ///< Radius for sphere/cylinder
    double newObjectHeight;         ///< Height for cylinder/cone
    double newObjectHalfAngle;      ///< Half-angle for cone (degrees)
    Vec3 newObjectColor;            ///< Color for new object's material
    double newObjectShininess;      ///< Shininess for new object
    double newObjectReflectivity;   ///< Reflectivity for new object

    // Object selection state
    size_t selectedObject;          ///< Index of selected object, or SIZE_MAX
    bool hasSelection;              ///< Whether an object is selected

    // Edit panel state
    Vec3 editPosition;              ///< Current edited position
    Vec3 editColor;                 ///< Current edited color
    double editShininess;           ///< Current edited shininess
    double editReflectivity;        ///< Current edited reflectivity
    double editAmbient;             ///< Current edited ambient
    double editDiffuse;             ///< Current edited diffuse
    double editSpecular;            ///< Current edited specular
    bool editModified;              ///< Whether edit values have been modified pending apply

    /**
     * @brief Update FPS counter
     */
    void updateFPS();

    /**
     * @brief Build the Render Settings panel
     */
    void buildRenderSettingsPanel();

    /**
     * @brief Build the Info overlay panel
     */
    void buildInfoOverlay();

    /**
     * @brief Build a toolbar at the top of the window
     */
    void buildToolbar();

    /**
     * @brief Build the Object Creation panel
     */
    void buildObjectCreationPanel();

    /**
     * @brief Build the Object Editing panel (for selected objects)
     */
    void buildObjectEditPanel();

    /**
     * @brief Create a new object in the scene with current creation params
     */
    void createObject();

    /**
     * @brief Apply current edit values to the selected object
     */
    void applyEditToObject();
};
