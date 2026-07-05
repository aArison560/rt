/**
 * @file GuiManager.cpp
 * @author RT Team - Dev C
 * @date 2026-07-05
 * @brief GuiManager implementation
 */

#include "gui/GuiManager.hpp"
#include <imgui.h>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include <SDL2/SDL.h>
#include "geometry/Sphere.hpp"
#include "geometry/Plane.hpp"
#include "geometry/Cylinder.hpp"
#include "geometry/Cone.hpp"
#include <format>
#include <iostream>

GuiManager::GuiManager(Window* win, Renderer* rend, EventHandler* evtHandler)
    : window(win)
    , renderer(rend)
    , eventHandler(evtHandler)
    , scene(nullptr)
    , visible(true)
    , initialized(false)
    , redrawRequested(false)
    , lastPerformanceCounter(SDL_GetPerformanceCounter())
    , frameCount(0)
    , fps(60.0f)
    , frameTimeMs(16.0f)
    , showCreatePanel(false)
    , newObjectType(0)
    , newObjectPos(0.0, 0.0, -3.0)
    , newObjectRadius(1.0)
    , newObjectHeight(2.0)
    , newObjectHalfAngle(25.0)
    , newObjectColor(0.9, 0.2, 0.2)
    , newObjectShininess(32.0)
    , newObjectReflectivity(0.0)
    , selectedObject(0)
    , hasSelection(false)
    , editPosition(0, 0, 0)
    , editColor(1, 1, 1)
    , editShininess(32.0)
    , editReflectivity(0.0)
    , editAmbient(0.1)
    , editDiffuse(0.7)
    , editSpecular(0.2)
    , editModified(false)
{
}

GuiManager::~GuiManager()
{
    shutdown();
}

bool GuiManager::initialize()
{
    if (initialized) {
        return true;
    }

    // Check that we have the required SDL2 objects
    SDL_Window* sdlWindow = window->getSDLWindow();
    SDL_Renderer* sdlRenderer = window->getSDLRenderer();
    if (!sdlWindow || !sdlRenderer) {
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Forward SDL events to ImGui before EventHandler processes them
    eventHandler->onSDLEvent([](const SDL_Event& event) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    });
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.WindowRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    // Setup platform/renderer backends
    if (!ImGui_ImplSDL2_InitForSDLRenderer(sdlWindow, sdlRenderer)) {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(sdlRenderer)) {
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    initialized = true;
    return true;
}

void GuiManager::shutdown()
{
    if (!initialized) {
        return;
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

void GuiManager::beginFrame()
{
    if (!initialized || !visible) {
        return;
    }

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    updateFPS();
}

void GuiManager::render()
{
    if (!initialized || !visible) {
        return;
    }

    // Main panels
    buildToolbar();
    buildRenderSettingsPanel();
    buildInfoOverlay();

    // Phase 3: Object creation
    buildObjectCreationPanel();

    // Phase 4: Object selection & editing
    buildObjectEditPanel();
}

void GuiManager::renderOverlay()
{
    if (!initialized || !visible) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

bool GuiManager::wantsCaptureKeyboard() const
{
    if (!initialized || !visible) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool GuiManager::wantsCaptureMouse() const
{
    if (!initialized || !visible) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

bool GuiManager::isVisible() const
{
    return visible;
}

void GuiManager::toggleVisibility()
{
    visible = !visible;
}

void GuiManager::setScene(Scene* s)
{
    scene = s;
}

bool GuiManager::needsRedraw() const
{
    return redrawRequested;
}

void GuiManager::clearRedrawFlag()
{
    redrawRequested = false;
}

void GuiManager::onMouseClick(int mouseX, int mouseY)
{
    if (!initialized || !visible || !scene) {
        return;
    }
    // Don't pick if ImGui is using the mouse (e.g. dragging a slider)
    if (wantsCaptureMouse()) {
        return;
    }

    int winW = window->getWidth();
    int winH = window->getHeight();
    if (mouseX < 0 || mouseX >= winW || mouseY < 0 || mouseY >= winH) {
        return;
    }

    PickResult result = renderer->pickObject(*scene, mouseX, mouseY, winW, winH);
    if (result.hit) {
        hasSelection = true;
        editModified = false; // Reset edit flag when selecting a new object
        selectedObject = result.objectIndex;

        // Close the creation panel when selecting an object
        showCreatePanel = false;

        // Load current properties into edit state
        if (selectedObject < scene->getObjectCount()) {
            auto obj = scene->getObject(selectedObject);
            auto mat = obj->getMaterial();
            // Read position from transform
            editPosition = obj->getTransform().getTranslation();
            if (mat) {
                editColor = mat->getColor();
                editShininess = mat->getShininess();
                editReflectivity = mat->getReflectivity();
                editAmbient = mat->getAmbient();
                editDiffuse = mat->getDiffuse();
                editSpecular = mat->getSpecular();
            }
        }
    } else {
        // Clicking empty space deselects
        hasSelection = false;
    }
}

// ---- Private helpers ----

void GuiManager::updateFPS()
{
    uint64_t currentCounter = SDL_GetPerformanceCounter();
    double deltaSeconds = static_cast<double>(currentCounter - lastPerformanceCounter)
                          / static_cast<double>(SDL_GetPerformanceFrequency());
    lastPerformanceCounter = currentCounter;

    if (deltaSeconds > 0.0) {
        double instantFps = 1.0 / deltaSeconds;
        constexpr float smoothing = 0.9f;
        fps = smoothing * fps + (1.0f - smoothing) * static_cast<float>(instantFps);
        frameTimeMs = deltaSeconds * 1000.0;
    }
}

void GuiManager::buildToolbar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f - 200.0f,
                                   viewport->Pos.y + 8.0f),
                            ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("##Toolbar", nullptr, flags)) {
        ImGui::Text("RT");
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        if (ImGui::Button("Rerender")) {
            redrawRequested = true;
        }
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        // Toggle create panel
        if (ImGui::Button(showCreatePanel ? "Close Create" : "+ Create")) {
            showCreatePanel = !showCreatePanel;
        }
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        ImGui::TextDisabled("H: hide | Click obj: select");
    }
    ImGui::End();
}

void GuiManager::buildRenderSettingsPanel()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0f, 0.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Render Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    int samples = renderer->getSamplesPerPixel();
    if (ImGui::SliderInt("Samples", &samples, 1, 64, "%d spp")) {
        renderer->setSamplesPerPixel(samples);
        redrawRequested = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Number of rays per pixel");
        ImGui::Text("Higher = smoother but slower");
        ImGui::EndTooltip();
    }

    int depth = renderer->getMaxRecursionDepth();
    if (ImGui::SliderInt("Max Depth", &depth, 1, 16, "%d bounces")) {
        renderer->setMaxRecursionDepth(depth);
        redrawRequested = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Max bounces for reflections/refractions");
        ImGui::EndTooltip();
    }

    ImGui::SeparatorText("Toggles");

    bool shadows = renderer->getShadowsEnabled();
    if (ImGui::Checkbox("Shadows", &shadows)) {
        renderer->setShadowsEnabled(shadows);
        redrawRequested = true;
    }

    bool reflections = renderer->getReflectionsEnabled();
    if (ImGui::Checkbox("Reflections", &reflections)) {
        renderer->setReflectionsEnabled(reflections);
        redrawRequested = true;
    }

    bool refractions = renderer->getRefractionsEnabled();
    if (ImGui::Checkbox("Refractions", &refractions)) {
        renderer->setRefractionsEnabled(refractions);
        redrawRequested = true;
    }

    ImGui::Separator();

    int shadowSamples = renderer->getShadowSamples();
    if (ImGui::SliderInt("Shadow Samples", &shadowSamples, 1, 32, "%d")) {
        renderer->setShadowSamples(shadowSamples);
        redrawRequested = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Number of light samples for soft shadows");
        ImGui::EndTooltip();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (ImGui::Button("Re-render Scene", ImVec2(-1.0f, 32.0f))) {
        redrawRequested = true;
    }

    double renderTime = renderer->getLastRenderTimeMs();
    if (renderTime > 0.0) {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
                          "Last render: %.1f ms", renderTime);
    }

    ImGui::End();
}

void GuiManager::buildInfoOverlay()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.45f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("##InfoOverlay", nullptr, flags)) {
        ImGui::Text("RT - Ray Tracer");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f  (%.1f ms)", fps, frameTimeMs);

        double renderTime = renderer->getLastRenderTimeMs();
        if (renderTime > 0.0) {
            ImGui::Text("Render: %.1f ms", renderTime);
        }

        if (scene) {
            ImGui::Text("Objects: %zu", scene->getObjectCount());
            ImGui::Text("Lights:  %zu", scene->getLightCount());
            if (!scene->getName().empty()) {
                ImGui::Text("Scene: %s", scene->getName().c_str());
            }
        }

        // Show selected object info
        if (hasSelection && scene && selectedObject < scene->getObjectCount()) {
            ImGui::Separator();
            auto obj = scene->getObject(selectedObject);
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                              "Selected: %s #%zu", obj->getType(), selectedObject);
        }

        ImGui::Separator();
        ImGui::Text("%d spp | depth %d", renderer->getSamplesPerPixel(),
                    renderer->getMaxRecursionDepth());
        ImGui::Text("%dx%d", window->getWidth(), window->getHeight());
    }
    ImGui::End();
}

void GuiManager::buildObjectCreationPanel()
{
    if (!showCreatePanel) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(310.0f, 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Create Object", &showCreatePanel, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    // Object type selector
    const char* types[] = { "Sphere", "Plane", "Cylinder", "Cone" };
    ImGui::Combo("Type", &newObjectType, types, IM_ARRAYSIZE(types));

    ImGui::SeparatorText("Position");
    float posF[3] = { (float)newObjectPos.x, (float)newObjectPos.y, (float)newObjectPos.z };
    if (ImGui::DragFloat3("Position", posF, 0.1f, -20.0f, 20.0f, "%.2f")) {
        newObjectPos = Vec3(posF[0], posF[1], posF[2]);
    }

    // Type-specific parameters
    float radiusF = (float)newObjectRadius;
    float heightF = (float)newObjectHeight;
    float halfAngleF = (float)newObjectHalfAngle;

    if (newObjectType == 0) { // Sphere
        if (ImGui::DragFloat("Radius", &radiusF, 0.05f, 0.1f, 10.0f, "%.2f"))
            newObjectRadius = radiusF;
    } else if (newObjectType == 2) { // Cylinder
        if (ImGui::DragFloat("Radius", &radiusF, 0.05f, 0.1f, 10.0f, "%.2f"))
            newObjectRadius = radiusF;
        if (ImGui::DragFloat("Height", &heightF, 0.05f, 0.1f, 20.0f, "%.2f"))
            newObjectHeight = heightF;
    } else if (newObjectType == 3) { // Cone
        if (ImGui::DragFloat("Height", &heightF, 0.05f, 0.1f, 20.0f, "%.2f"))
            newObjectHeight = heightF;
        if (ImGui::DragFloat("Half Angle", &halfAngleF, 1.0f, 1.0f, 89.0f, "%.0f deg"))
            newObjectHalfAngle = halfAngleF;
    }

    ImGui::SeparatorText("Material");
    float color[3] = { (float)newObjectColor.x, (float)newObjectColor.y, (float)newObjectColor.z };
    if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_NoInputs)) {
        newObjectColor = Vec3(color[0], color[1], color[2]);
    }
    float shinyF = (float)newObjectShininess;
    if (ImGui::DragFloat("Shininess", &shinyF, 0.5f, 0.0f, 256.0f, "%.1f"))
        newObjectShininess = shinyF;
    float reflF = (float)newObjectReflectivity;
    if (ImGui::DragFloat("Reflectivity", &reflF, 0.01f, 0.0f, 1.0f, "%.2f"))
        newObjectReflectivity = reflF;

    ImGui::Separator();

    // Create button
    if (ImGui::Button("Create Object", ImVec2(-1.0f, 32.0f))) {
        createObject();
    }

    ImGui::End();
}

void GuiManager::buildObjectEditPanel()
{
    if (!hasSelection || !scene || selectedObject >= scene->getObjectCount()) {
        return;
    }

    auto obj = scene->getObject(selectedObject);

    ImGui::SetNextWindowPos(ImVec2(310.0f, 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f), ImGuiCond_FirstUseEver);

    std::string title = std::format("Edit {} #{}", obj->getType(), selectedObject);
    if (!ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    // Show object type and index
    ImGui::Text("Type: %s", obj->getType());
    ImGui::Text("Index: %zu", selectedObject);
    ImGui::Separator();

    // --- Position ---
    ImGui::SeparatorText("Transform");
    float posF[3] = { (float)editPosition.x, (float)editPosition.y, (float)editPosition.z };
    if (ImGui::DragFloat3("Position", posF, 0.1f, -20.0f, 20.0f, "%.2f")) {
        editPosition = Vec3(posF[0], posF[1], posF[2]);
        editModified = true;
    }

    // --- Material ---
    ImGui::SeparatorText("Material");
    float col[3] = { (float)editColor.x, (float)editColor.y, (float)editColor.z };
    if (ImGui::ColorEdit3("Color", col, ImGuiColorEditFlags_NoInputs)) {
        editColor = Vec3(col[0], col[1], col[2]);
        editModified = true;
    }

    float ambF = (float)editAmbient;
    float diffF = (float)editDiffuse;
    float specF = (float)editSpecular;
    float shinyF = (float)editShininess;
    float reflF = (float)editReflectivity;

    if (ImGui::DragFloat("Ambient", &ambF, 0.01f, 0.0f, 1.0f, "%.2f")) {
        editAmbient = ambF; editModified = true;
    }
    if (ImGui::DragFloat("Diffuse", &diffF, 0.01f, 0.0f, 1.0f, "%.2f")) {
        editDiffuse = diffF; editModified = true;
    }
    if (ImGui::DragFloat("Specular", &specF, 0.01f, 0.0f, 1.0f, "%.2f")) {
        editSpecular = specF; editModified = true;
    }
    if (ImGui::DragFloat("Shininess", &shinyF, 0.5f, 0.0f, 256.0f, "%.1f")) {
        editShininess = shinyF; editModified = true;
    }
    if (ImGui::DragFloat("Reflectivity", &reflF, 0.01f, 0.0f, 1.0f, "%.2f")) {
        editReflectivity = reflF; editModified = true;
    }

    // Apply button
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (editModified) {
        if (ImGui::Button("Apply Changes", ImVec2(-1.0f, 32.0f))) {
            applyEditToObject();
            editModified = false;
        }
    } else {
        ImGui::TextDisabled("(modify values above to apply)");
    }

    // Delete button
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete Object", ImVec2(-1.0f, 32.0f))) {
        if (scene->removeObject(obj)) {
            hasSelection = false;
            redrawRequested = true;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
}

void GuiManager::createObject()
{
    if (!scene) return;

    auto mat = std::make_shared<Material>(
        newObjectColor,
        0.1,     // ambient
        0.7,     // diffuse
        0.2,     // specular
        newObjectShininess
    );
    mat->setReflectivity(newObjectReflectivity);

    std::shared_ptr<AObject> obj;

    switch (newObjectType) {
        case 0: { // Sphere
            obj = std::make_shared<Sphere>(newObjectPos, newObjectRadius, mat);
            break;
        }
        case 1: { // Plane
            obj = std::make_shared<Plane>(newObjectPos, Vec3(0.0, 1.0, 0.0), mat);
            break;
        }
        case 2: { // Cylinder
            obj = std::make_shared<Cylinder>(
                newObjectPos, Vec3(0.0, 1.0, 0.0),
                newObjectRadius, newObjectHeight, mat
            );
            break;
        }
        case 3: { // Cone
            Vec3 apex = newObjectPos + Vec3(0.0, newObjectHeight, 0.0);
            obj = std::make_shared<Cone>(
                apex, Vec3(0.0, -1.0, 0.0),
                newObjectHalfAngle, newObjectHeight, mat
            );
            break;
        }
    }

    if (obj) {
        scene->addObject(obj);
        redrawRequested = true;
        hasSelection = true;
        selectedObject = scene->getObjectCount() - 1;
        editPosition = newObjectPos;
        editColor = newObjectColor;
        editShininess = newObjectShininess;
        editReflectivity = newObjectReflectivity;
    }
}

void GuiManager::applyEditToObject()
{
    if (!hasSelection || !scene || selectedObject >= scene->getObjectCount()) return;

    auto obj = scene->getObject(selectedObject);
    auto mat = obj->getMaterial();

    // Update material
    if (mat) {
        mat->setColor(editColor);
        mat->setShininess(editShininess);
        mat->setReflectivity(editReflectivity);
        mat->setAmbient(editAmbient);
        mat->setDiffuse(editDiffuse);
        mat->setSpecular(editSpecular);
    }

    // Update position via transform
    Transform t;
    t.translate(editPosition);
    obj->setTransform(t);

    redrawRequested = true;
}
