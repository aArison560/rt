/**
 * @file GuiManager.cpp
 * @author RT Team - Dev C
 * @date 2026-07-05
 * @brief GuiManager implementation using microui
 */

#include "gui/GuiManager.hpp"
#include "gui/microui.h"
#include "gui/r_render.h"
#include <SDL2/SDL.h>
#include "geometry/Sphere.hpp"
#include "geometry/Plane.hpp"
#include "geometry/Cylinder.hpp"
#include "geometry/Cone.hpp"
#include <format>
#include <iostream>
#include <cstring>

GuiManager::GuiManager(Window* win, Renderer* rend, EventHandler* evtHandler)
    : window(win)
    , renderer(rend)
    , eventHandler(evtHandler)
    , scene(nullptr)
    , visible(true)
    , initialized(false)
    , redrawRequested(false)
    , muCtx(nullptr)
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

    SDL_Renderer* sdlRenderer = window->getSDLRenderer();
    if (!sdlRenderer) {
        return false;
    }

    muCtx = new mu_Context;
    mu_init(muCtx);

    if (!r_init(muCtx, sdlRenderer)) {
        delete muCtx;
        muCtx = nullptr;
        return false;
    }

    /* Forward SDL events to microui before EventHandler processes them */
    eventHandler->onSDLEvent([this](const SDL_Event& event) {
        if (!muCtx) return;
        switch (event.type) {
            case SDL_MOUSEMOTION:
                mu_input_mousemove(muCtx, event.motion.x, event.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                    mu_input_mousedown(muCtx, event.button.x, event.button.y, MU_MOUSE_LEFT);
                else if (event.button.button == SDL_BUTTON_RIGHT)
                    mu_input_mousedown(muCtx, event.button.x, event.button.y, MU_MOUSE_RIGHT);
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                    mu_input_mousedown(muCtx, event.button.x, event.button.y, MU_MOUSE_MIDDLE);
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT)
                    mu_input_mouseup(muCtx, event.button.x, event.button.y, MU_MOUSE_LEFT);
                else if (event.button.button == SDL_BUTTON_RIGHT)
                    mu_input_mouseup(muCtx, event.button.x, event.button.y, MU_MOUSE_RIGHT);
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                    mu_input_mouseup(muCtx, event.button.x, event.button.y, MU_MOUSE_MIDDLE);
                break;
            case SDL_MOUSEWHEEL:
                mu_input_scroll(muCtx, 0, event.wheel.y);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
                    mu_input_keydown(muCtx, MU_KEY_SHIFT);
                else if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
                    mu_input_keydown(muCtx, MU_KEY_CTRL);
                else if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
                    mu_input_keydown(muCtx, MU_KEY_ALT);
                else if (event.key.keysym.sym == SDLK_BACKSPACE)
                    mu_input_keydown(muCtx, MU_KEY_BACKSPACE);
                else if (event.key.keysym.sym == SDLK_RETURN)
                    mu_input_keydown(muCtx, MU_KEY_RETURN);
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
                    mu_input_keyup(muCtx, MU_KEY_SHIFT);
                else if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
                    mu_input_keyup(muCtx, MU_KEY_CTRL);
                else if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
                    mu_input_keyup(muCtx, MU_KEY_ALT);
                else if (event.key.keysym.sym == SDLK_BACKSPACE)
                    mu_input_keyup(muCtx, MU_KEY_BACKSPACE);
                else if (event.key.keysym.sym == SDLK_RETURN)
                    mu_input_keyup(muCtx, MU_KEY_RETURN);
                break;
            case SDL_TEXTINPUT:
                mu_input_text(muCtx, event.text.text);
                break;
        }
    });

    initialized = true;
    return true;
}

void GuiManager::shutdown()
{
    if (!initialized) {
        return;
    }
    if (muCtx) {
        delete muCtx;
        muCtx = nullptr;
    }
    initialized = false;
}

void GuiManager::beginFrame()
{
    if (!initialized || !visible || !muCtx) {
        return;
    }
    mu_begin(muCtx);
    updateFPS();
}

void GuiManager::render()
{
    if (!initialized || !visible || !muCtx) {
        return;
    }

    buildToolbar();
    buildRenderSettingsPanel();
    buildInfoOverlay();
    buildObjectCreationPanel();
    buildObjectEditPanel();
}

void GuiManager::renderOverlay()
{
    if (!initialized || !visible || !muCtx) {
        return;
    }

    mu_end(muCtx);

    SDL_Renderer* sdlRenderer = window->getSDLRenderer();
    if (sdlRenderer) {
        r_render(muCtx, sdlRenderer);
    }
}

bool GuiManager::wantsCaptureKeyboard() const
{
    if (!initialized || !visible || !muCtx) {
        return false;
    }
    return muCtx->focus != 0;
}

bool GuiManager::wantsCaptureMouse() const
{
    if (!initialized || !visible || !muCtx) {
        return false;
    }
    return muCtx->focus != 0 || muCtx->hover != 0;
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
        editModified = false;
        selectedObject = result.objectIndex;
        showCreatePanel = false;

        if (selectedObject < scene->getObjectCount()) {
            auto obj = scene->getObject(selectedObject);
            auto mat = obj->getMaterial();
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
        hasSelection = false;
    }
}

/* ========================================================================= */
/* Private helpers                                                           */
/* ========================================================================= */

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
    int w = window->getWidth();
    int tw = 400;
    int tx = (w - tw) / 2;

    if (!mu_begin_window_ex(muCtx, "##Toolbar", mu_rect(tx, 4, tw, 28),
            MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE)) {
        return;
    }

    int cols[] = {30, 8, 80, 8, 100, 8, 90};
    mu_layout_row(muCtx, 7, cols, 0);

    mu_label(muCtx, "RT");
    mu_label(muCtx, "|");
    if (mu_button(muCtx, "Rerender")) {
        redrawRequested = true;
    }
    mu_label(muCtx, "|");
    if (mu_button(muCtx, showCreatePanel ? "Close Create" : "+ Create")) {
        showCreatePanel = !showCreatePanel;
    }
    mu_label(muCtx, "|");
    mu_label(muCtx, "H: hide");

    mu_end_window(muCtx);
}

void GuiManager::buildRenderSettingsPanel()
{
    int pw = 240;

    if (!mu_begin_window(muCtx, "Render Settings", mu_rect(8, 34, pw, 300))) {
        return;
    }

    int pw2 = -muCtx->style->padding * 2;
    int c2[] = {pw2, 70};
    mu_layout_row(muCtx, 2, c2, 0);

    /* Samples */
    mu_label(muCtx, "Samples");
    mu_Real samplesR = (mu_Real)renderer->getSamplesPerPixel();
    if (mu_slider(muCtx, &samplesR, 1.0f, 64.0f)) {
        renderer->setSamplesPerPixel((int)samplesR);
        redrawRequested = true;
    }

    /* Max Depth */
    mu_label(muCtx, "Max Depth");
    mu_Real depthR = (mu_Real)renderer->getMaxRecursionDepth();
    if (mu_slider(muCtx, &depthR, 1.0f, 16.0f)) {
        renderer->setMaxRecursionDepth((int)depthR);
        redrawRequested = true;
    }

    /* Toggles */
    mu_label(muCtx, "--- Toggles ---");

    int shadows = renderer->getShadowsEnabled() ? 1 : 0;
    if (mu_checkbox(muCtx, "Shadows", &shadows)) {
        renderer->setShadowsEnabled(shadows != 0);
        redrawRequested = true;
    }

    int reflections = renderer->getReflectionsEnabled() ? 1 : 0;
    if (mu_checkbox(muCtx, "Reflections", &reflections)) {
        renderer->setReflectionsEnabled(reflections != 0);
        redrawRequested = true;
    }

    int refractions = renderer->getRefractionsEnabled() ? 1 : 0;
    if (mu_checkbox(muCtx, "Refractions", &refractions)) {
        renderer->setRefractionsEnabled(refractions != 0);
        redrawRequested = true;
    }

    /* Shadow Samples */
    mu_label(muCtx, "Shadow Smp");
    mu_Real shadowSamplesR = (mu_Real)renderer->getShadowSamples();
    if (mu_slider(muCtx, &shadowSamplesR, 1.0f, 32.0f)) {
        renderer->setShadowSamples((int)shadowSamplesR);
        redrawRequested = true;
    }

    /* Re-render button */
    int c3[] = {-1};
    mu_layout_row(muCtx, 1, c3, 28);
    if (mu_button(muCtx, "Re-render Scene")) {
        redrawRequested = true;
    }

    /* Render time */
    double renderTime = renderer->getLastRenderTimeMs();
    if (renderTime > 0.0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Last render: %.1f ms", renderTime);
        mu_label(muCtx, buf);
    }

    mu_end_window(muCtx);
}

void GuiManager::buildInfoOverlay()
{
    if (!mu_begin_window_ex(muCtx, "##Info", mu_rect(8, 270, 220, 0),
            MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE)) {
        return;
    }

    int c2[] = {-1};
    mu_layout_row(muCtx, 1, c2, 0);
    mu_label(muCtx, "RT - Ray Tracer");

    char buf[128];
    snprintf(buf, sizeof(buf), "FPS: %.1f  (%.1f ms)", fps, frameTimeMs);
    mu_label(muCtx, buf);

    double renderTime = renderer->getLastRenderTimeMs();
    if (renderTime > 0.0) {
        snprintf(buf, sizeof(buf), "Render: %.1f ms", renderTime);
        mu_label(muCtx, buf);
    }

    if (scene) {
        snprintf(buf, sizeof(buf), "Objects: %zu", scene->getObjectCount());
        mu_label(muCtx, buf);
        snprintf(buf, sizeof(buf), "Lights: %zu", scene->getLightCount());
        mu_label(muCtx, buf);
        if (!scene->getName().empty()) {
            snprintf(buf, sizeof(buf), "Scene: %s", scene->getName().c_str());
            mu_label(muCtx, buf);
        }
    }

    if (hasSelection && scene && selectedObject < scene->getObjectCount()) {
        auto obj = scene->getObject(selectedObject);
        snprintf(buf, sizeof(buf), "Selected: %s #%zu", obj->getType(), selectedObject);
        mu_label(muCtx, buf);
    }

    snprintf(buf, sizeof(buf), "%d spp | depth %d",
             renderer->getSamplesPerPixel(), renderer->getMaxRecursionDepth());
    mu_label(muCtx, buf);
    snprintf(buf, sizeof(buf), "%dx%d", window->getWidth(), window->getHeight());
    mu_label(muCtx, buf);

    mu_end_window(muCtx);
}

void GuiManager::buildObjectCreationPanel()
{
    if (!showCreatePanel) {
        return;
    }

    if (!mu_begin_window(muCtx, "Create Object", mu_rect(300, 34, 280, 320))) {
        return;
    }

    /* Object type selector */
    const char* types[] = { "Sphere", "Plane", "Cylinder", "Cone" };
    int c1[] = {70, -1};
    mu_layout_row(muCtx, 2, c1, 0);
    mu_label(muCtx, "Type");
    if (mu_button(muCtx, types[newObjectType])) {
        newObjectType = (newObjectType + 1) % 4;
    }

    /* Position */
    mu_label(muCtx, "--- Position ---");
    int c2[] = {30, -1};
    mu_layout_row(muCtx, 2, c2, 0);
    mu_label(muCtx, "X");
    mu_Real posx = (mu_Real)newObjectPos.x;
    if (mu_slider(muCtx, &posx, -20.0f, 20.0f)) {
        newObjectPos.x = posx;
    }
    mu_label(muCtx, "Y");
    mu_Real posy = (mu_Real)newObjectPos.y;
    if (mu_slider(muCtx, &posy, -20.0f, 20.0f)) {
        newObjectPos.y = posy;
    }
    mu_label(muCtx, "Z");
    mu_Real posz = (mu_Real)newObjectPos.z;
    if (mu_slider(muCtx, &posz, -20.0f, 20.0f)) {
        newObjectPos.z = posz;
    }

    /* Type-specific parameters */
    if (newObjectType == 0) { /* Sphere */
        mu_label(muCtx, "Radius");
        mu_Real r = (mu_Real)newObjectRadius;
        if (mu_slider(muCtx, &r, 0.1f, 10.0f)) {
            newObjectRadius = r;
        }
    } else if (newObjectType == 2) { /* Cylinder */
        mu_label(muCtx, "Radius");
        mu_Real r = (mu_Real)newObjectRadius;
        if (mu_slider(muCtx, &r, 0.1f, 10.0f)) {
            newObjectRadius = r;
        }
        mu_label(muCtx, "Height");
        mu_Real h = (mu_Real)newObjectHeight;
        if (mu_slider(muCtx, &h, 0.1f, 20.0f)) {
            newObjectHeight = h;
        }
    } else if (newObjectType == 3) { /* Cone */
        mu_label(muCtx, "Height");
        mu_Real h = (mu_Real)newObjectHeight;
        if (mu_slider(muCtx, &h, 0.1f, 20.0f)) {
            newObjectHeight = h;
        }
        mu_label(muCtx, "Half Angle");
        mu_Real ha = (mu_Real)newObjectHalfAngle;
        if (mu_slider(muCtx, &ha, 1.0f, 89.0f)) {
            newObjectHalfAngle = ha;
        }
    }

    /* Material */
    mu_label(muCtx, "--- Material ---");

    int c3[] = {30, -1};
    mu_layout_row(muCtx, 2, c3, 0);
    mu_label(muCtx, "R");
    mu_Real cr = (mu_Real)newObjectColor.x;
    if (mu_slider(muCtx, &cr, 0.0f, 1.0f)) {
        newObjectColor.x = cr;
    }
    mu_label(muCtx, "G");
    mu_Real cg = (mu_Real)newObjectColor.y;
    if (mu_slider(muCtx, &cg, 0.0f, 1.0f)) {
        newObjectColor.y = cg;
    }
    mu_label(muCtx, "B");
    mu_Real cb = (mu_Real)newObjectColor.z;
    if (mu_slider(muCtx, &cb, 0.0f, 1.0f)) {
        newObjectColor.z = cb;
    }

    mu_label(muCtx, "Shininess");
    mu_Real sh = (mu_Real)newObjectShininess;
    if (mu_slider(muCtx, &sh, 0.0f, 256.0f)) {
        newObjectShininess = sh;
    }

    mu_label(muCtx, "Reflect.");
    mu_Real rf = (mu_Real)newObjectReflectivity;
    if (mu_slider(muCtx, &rf, 0.0f, 1.0f)) {
        newObjectReflectivity = rf;
    }

    /* Create button */
    int c4[] = {-1};
    mu_layout_row(muCtx, 1, c4, 28);
    if (mu_button(muCtx, "Create Object")) {
        createObject();
    }

    mu_end_window(muCtx);
}

void GuiManager::buildObjectEditPanel()
{
    if (!hasSelection || !scene || selectedObject >= scene->getObjectCount()) {
        return;
    }

    auto obj = scene->getObject(selectedObject);

    char title[64];
    snprintf(title, sizeof(title), "Edit %s #%zu", obj->getType(), selectedObject);

    if (!mu_begin_window(muCtx, title, mu_rect(300, 34, 280, 340))) {
        return;
    }

    char buf[128];
    int c1[] = {-1};
    mu_layout_row(muCtx, 1, c1, 0);

    snprintf(buf, sizeof(buf), "Type: %s", obj->getType());
    mu_label(muCtx, buf);
    snprintf(buf, sizeof(buf), "Index: %zu", selectedObject);
    mu_label(muCtx, buf);

    /* Position */
    mu_label(muCtx, "--- Transform ---");
    int c2[] = {30, -1};
    mu_layout_row(muCtx, 2, c2, 0);

    mu_label(muCtx, "X");
    mu_Real posx = (mu_Real)editPosition.x;
    if (mu_slider(muCtx, &posx, -20.0f, 20.0f)) {
        editPosition.x = posx;
        editModified = true;
    }
    mu_label(muCtx, "Y");
    mu_Real posy = (mu_Real)editPosition.y;
    if (mu_slider(muCtx, &posy, -20.0f, 20.0f)) {
        editPosition.y = posy;
        editModified = true;
    }
    mu_label(muCtx, "Z");
    mu_Real posz = (mu_Real)editPosition.z;
    if (mu_slider(muCtx, &posz, -20.0f, 20.0f)) {
        editPosition.z = posz;
        editModified = true;
    }

    /* Material */
    mu_label(muCtx, "--- Material ---");

    mu_label(muCtx, "R");
    mu_Real cr = (mu_Real)editColor.x;
    if (mu_slider(muCtx, &cr, 0.0f, 1.0f)) {
        editColor.x = cr;
        editModified = true;
    }
    mu_label(muCtx, "G");
    mu_Real cg = (mu_Real)editColor.y;
    if (mu_slider(muCtx, &cg, 0.0f, 1.0f)) {
        editColor.y = cg;
        editModified = true;
    }
    mu_label(muCtx, "B");
    mu_Real cb = (mu_Real)editColor.z;
    if (mu_slider(muCtx, &cb, 0.0f, 1.0f)) {
        editColor.z = cb;
        editModified = true;
    }

    mu_label(muCtx, "Ambient");
    mu_Real amb = (mu_Real)editAmbient;
    if (mu_slider(muCtx, &amb, 0.0f, 1.0f)) {
        editAmbient = amb;
        editModified = true;
    }
    mu_label(muCtx, "Diffuse");
    mu_Real diff = (mu_Real)editDiffuse;
    if (mu_slider(muCtx, &diff, 0.0f, 1.0f)) {
        editDiffuse = diff;
        editModified = true;
    }
    mu_label(muCtx, "Specular");
    mu_Real spec = (mu_Real)editSpecular;
    if (mu_slider(muCtx, &spec, 0.0f, 1.0f)) {
        editSpecular = spec;
        editModified = true;
    }
    mu_label(muCtx, "Shininess");
    mu_Real sh = (mu_Real)editShininess;
    if (mu_slider(muCtx, &sh, 0.0f, 256.0f)) {
        editShininess = sh;
        editModified = true;
    }
    mu_label(muCtx, "Reflect.");
    mu_Real rf = (mu_Real)editReflectivity;
    if (mu_slider(muCtx, &rf, 0.0f, 1.0f)) {
        editReflectivity = rf;
        editModified = true;
    }

    /* Apply button */
    int c3[] = {-1};
    mu_layout_row(muCtx, 1, c3, 28);
    if (editModified) {
        if (mu_button(muCtx, "Apply Changes")) {
            applyEditToObject();
            editModified = false;
        }
    } else {
        mu_label(muCtx, "(modify values above to apply)");
    }

    /* Delete button */
    mu_label(muCtx, "");
    int c4[] = {-1};
    mu_layout_row(muCtx, 1, c4, 28);
    if (mu_button(muCtx, "Delete Object")) {
        if (scene->removeObject(obj)) {
            hasSelection = false;
            redrawRequested = true;
        }
    }

    mu_end_window(muCtx);
}

void GuiManager::createObject()
{
    if (!scene) return;

    auto mat = std::make_shared<Material>(
        newObjectColor,
        0.1,
        0.7,
        0.2,
        newObjectShininess
    );
    mat->setReflectivity(newObjectReflectivity);

    std::shared_ptr<AObject> obj;

    switch (newObjectType) {
        case 0: {
            obj = std::make_shared<Sphere>(newObjectPos, newObjectRadius, mat);
            break;
        }
        case 1: {
            obj = std::make_shared<Plane>(newObjectPos, Vec3(0.0, 1.0, 0.0), mat);
            break;
        }
        case 2: {
            obj = std::make_shared<Cylinder>(
                newObjectPos, Vec3(0.0, 1.0, 0.0),
                newObjectRadius, newObjectHeight, mat
            );
            break;
        }
        case 3: {
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

    if (mat) {
        mat->setColor(editColor);
        mat->setShininess(editShininess);
        mat->setReflectivity(editReflectivity);
        mat->setAmbient(editAmbient);
        mat->setDiffuse(editDiffuse);
        mat->setSpecular(editSpecular);
    }

    Transform t;
    t.translate(editPosition);
    obj->setTransform(t);

    redrawRequested = true;
}
