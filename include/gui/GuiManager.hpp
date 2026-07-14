/**
 * @file GuiManager.hpp
 * @author RT Team - Dev C
 * @date 2026-07-05
 * @brief microui-based GUI overlay manager
 *
 * Manages the microui lifecycle and renders interactive UI panels
 * over the ray traced image.
 *
 * @relationships
 * - Uses: Window (SDL2 window/renderer), Renderer (settings + ray-picking), EventHandler (events)
 * - Used by: main.cpp
 * - Integrates: microui with SDL2 renderer backend
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

struct mu_Context;

class GuiManager
{
public:
    GuiManager(Window* window, Renderer* renderer, EventHandler* eventHandler);
    ~GuiManager();

    [[nodiscard]] bool initialize();
    void shutdown();
    void beginFrame();
    void render();
    void renderOverlay();

    [[nodiscard]] bool wantsCaptureKeyboard() const;
    [[nodiscard]] bool wantsCaptureMouse() const;
    [[nodiscard]] bool isVisible() const;
    void toggleVisibility();

    void setScene(Scene* scene);
    [[nodiscard]] bool needsRedraw() const;
    void clearRedrawFlag();

    void onMouseClick(int mouseX, int mouseY);

private:
    Window* window;
    Renderer* renderer;
    EventHandler* eventHandler;
    Scene* scene;
    bool visible;
    bool initialized;
    bool redrawRequested;

    mu_Context* muCtx;

    uint64_t lastPerformanceCounter;
    int frameCount;
    float fps;
    double frameTimeMs;

    bool showCreatePanel;
    int newObjectType;
    Vec3 newObjectPos;
    double newObjectRadius;
    double newObjectHeight;
    double newObjectHalfAngle;
    Vec3 newObjectColor;
    double newObjectShininess;
    double newObjectReflectivity;

    size_t selectedObject;
    bool hasSelection;

    Vec3 editPosition;
    Vec3 editColor;
    double editShininess;
    double editReflectivity;
    double editAmbient;
    double editDiffuse;
    double editSpecular;
    bool editModified;

    void updateFPS();
    void buildRenderSettingsPanel();
    void buildInfoOverlay();
    void buildToolbar();
    void buildObjectCreationPanel();
    void buildObjectEditPanel();
    void createObject();
    void applyEditToObject();
};
