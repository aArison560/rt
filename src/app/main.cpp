/**
 * @file main.cpp
 * @author RT Team - Dev C
 * @date 2026-04-01
 * @brief Ray tracer main entry point
 *
 * Usage: rt [scene_file] [width] [height]
 *
 * If no arguments provided, loads default scene.
 */

#include <iostream>
#include <format>
#include <filesystem>
#include <string>
#include <memory>
#include <vector>
#include "scene/Scene.hpp"
#include "scene/ManageFile.hpp"
#include "scene/Camera.hpp"
#include "rendering/Renderer.hpp"
#include "platform/Window.hpp"
#include "platform/EventHandler.hpp"
#include <SDL2/SDL.h>
#include "rendering/ImageBuffer.hpp"
#include "scene/SceneParser.hpp"
#include "geometry/Sphere.hpp"
#include "geometry/Plane.hpp"
#include "lighting/PointLight.hpp"
#include "lighting/AmbientLight.hpp"
#include "gui/GuiManager.hpp"

// Default constants
constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
const std::string DEFAULT_SCENE_FILE = "scenes/default.rt";

/**
 * @brief Create a default test scene
 * @return Populated Scene object
 */
Scene createDefaultScene()
{
    Scene scene;
    scene.setName("Default Scene");
    scene.setBackgroundColor(Vec3(0.35, 0.45, 0.6));

    // Simple default scene: one sphere, ambient + point light, camera
    scene.setName("Default Scene");

    // Camera
    Camera cam(Vec3(0.0, 1.5, 8.0), Vec3(0.0, -0.2, -1.0), Vec3(0.0, 1.0, 0.0), 50.0);
    scene.setCamera(cam);

    // Sphere material and object
    auto mat = std::make_shared<Material>(Vec3(0.9, 0.2, 0.2), 0.1, 0.7, 0.2, 32.0);
    auto sphere = std::make_shared<Sphere>(Vec3(0.0, 0.0, -3.0), 1.5, mat);
    scene.addObject(sphere);

    // Ambient light
    auto amb = std::make_shared<AmbientLight>(Vec3(1.0, 1.0, 1.0), 0.2);
    scene.addLight(amb);

    // Point light
    auto pl = std::make_shared<PointLight>(Vec3(5.0, 5.0, 5.0), Vec3(1.0, 1.0, 1.0), 1.0);
    pl->setAttenuation(1.0, 0.0, 0.0);
    scene.addLight(pl);

    return scene;
}

/**
 * @brief Print usage information
 * @param programName Name of the executable
 */
void printUsage(const std::string& programName)
{
    std::cout << std::format("Usage: {} [scene_file] [width] [height]\n"
              "  scene_file: Path to .rt scene file (default: {})\n"
              "  width:      Output width in pixels (default: {})\n"
              "  height:     Output height in pixels (default: {})\n"
              "\nExample: {} scenes/test.rt 1920 1080\n",
              programName, DEFAULT_SCENE_FILE, DEFAULT_WIDTH, DEFAULT_HEIGHT, programName);
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[])
{
    std::filesystem::path sceneFile = DEFAULT_SCENE_FILE;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;

    // Check for --profile flag before parsing positional arguments
    bool profilingEnabled = false;
    std::vector<std::string> positionalArgs;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--profile") {
            profilingEnabled = true;
        } else {
            positionalArgs.push_back(arg);
        }
    }

    // manage error
    try
    {
        ManageFile fl(argc, argv);
        fl.checkErrorFile();
    }
    catch(const std::exception& e)
    {
        return (-1);
    }

    // Parse positional arguments
    if (positionalArgs.size() >= 1) {
        sceneFile = positionalArgs[0];
    }
    if (positionalArgs.size() >= 2) {
        try {
            width = std::stoi(positionalArgs[1]);
        } catch (...) {
            std::cerr << "Invalid width: " << positionalArgs[1] << std::endl;
            return 1;
        }
    }
    if (positionalArgs.size() >= 3) {
        try {
            height = std::stoi(positionalArgs[2]);
        } catch (...) {
            std::cerr << "Invalid height: " << positionalArgs[2] << std::endl;
            return 1;
        }
    }

    Scene scene;
    SceneParser parser;
    if (!parser.parseFile(sceneFile, scene)) {
        std::cerr << "Warning: Could not load scene from " << sceneFile << "\n"
                  << "Error: " << parser.getLastError() << "\n"
                  << "Creating default scene instead.\n";
        scene = createDefaultScene();
    } else {
        std::cout << "Loaded scene: " << scene.getName() << std::endl;
    }

    Window window("RT - Ray Tracer", width, height);
    if (!window.isInitialized()) {
        std::cerr << "Failed to initialize window\n";
        return 1;
    }
    std::cout << std::format("Window initialized: {}x{}", width, height) << std::endl;

    Renderer renderer;
    renderer.setProfilingEnabled(profilingEnabled);
    renderer.setMaxRecursionDepth(4);
    renderer.setShadowsEnabled(true);
    renderer.setReflectionsEnabled(true);
    renderer.setRefractionsEnabled(true);
    renderer.setSamplesPerPixel(8);

    ImageBuffer buffer(width, height);

    // Main render loop state
    bool running = true;
    bool needsRedraw = true;
    int screenshotCount = 0;

    // Setup event handler
    EventHandler eventHandler;
    eventHandler.setupCameraControls(&scene.getCamera(), 0.5, 0.05);

    // Setup GUI manager
    GuiManager guiManager(&window, &renderer, &eventHandler);
    guiManager.setScene(&scene);

    // Handle mouse clicks for object picking
    eventHandler.onMouseClick([&](int mouseX, int mouseY, int button) {
        if (button == SDL_BUTTON_LEFT) {
            guiManager.onMouseClick(mouseX, mouseY);
            needsRedraw = true;
        }
    });

    // Initialize ImGui (also sets up the SDL event callback internally)
    if (!guiManager.initialize()) {
        std::cerr << "Warning: Failed to initialize GUI overlay (continuing without)\n";
    }

    std::cout << "Starting render loop...\n";
    std::cout << "Controls: WASD=move, Q/E=up/down, Arrows=rotate, "
              << "S=screenshot, R=re-render, H=toggle UI\n";

    eventHandler.onKeyPress([&](int key) {
        if (key == 's' || key == 'S') {
            auto filename = std::format("screenshot_{}.png", screenshotCount++);
            if (buffer.savePNG(filename)) {
                std::cout << "Saved: " << filename << std::endl;
            } else {
                std::cerr << "Failed to save screenshot" << std::endl;
            }
        }
        if (key == 'r' || key == 'R') {
            needsRedraw = true;
        }
        if (key == 'h' || key == 'H') {
            guiManager.toggleVisibility();
            std::cout << "GUI " << (guiManager.isVisible() ? "shown" : "hidden") << std::endl;
        }
    });

    auto oldCamPos = scene.getCamera().getPosition();
    auto oldCamDir = scene.getCamera().getDirection();

    while (running) {
        // Disable camera controls when ImGui wants keyboard input
        eventHandler.setCameraControlEnabled(!guiManager.wantsCaptureKeyboard());

        // Process events
        if (eventHandler.pollEvents()) {
            running = false;
            break;
        }

        // Begin ImGui frame (must be before any SDL_Renderer operations)
        guiManager.beginFrame();

        // Build GUI panels
        guiManager.render();

        // Check if GUI requested a re-render
        if (guiManager.needsRedraw()) {
            needsRedraw = true;
            guiManager.clearRedrawFlag();
        }

        // Detect camera movement from WASD/arrows → re-render
        auto newCamPos = scene.getCamera().getPosition();
        auto newCamDir = scene.getCamera().getDirection();
        if ((newCamPos - oldCamPos).magnitudeSquared() > Vec3::EPSILON * Vec3::EPSILON ||
            (newCamDir - oldCamDir).magnitudeSquared() > Vec3::EPSILON * Vec3::EPSILON) {
            needsRedraw = true;
            oldCamPos = newCamPos;
            oldCamDir = newCamDir;
        }

        // Re-render only when needed (camera movement, resize, etc.)
        if (needsRedraw) {
            std::cout << "Rendering scene...\n";
            if (!renderer.render(scene, width, height, buffer.getData())) {
                std::cerr << "Render failed\n";
                running = false;
                break;
            }
            needsRedraw = false;
        }

        // Always refresh the display (handles window expose/uncover)
        if (!window.updateDisplay(buffer)) {
            std::cerr << "Failed to update display\n";
            running = false;
            break;
        }

        // Render ImGui overlay on top of the ray traced image
        guiManager.renderOverlay();

        // Present the final frame (raytracing + GUI overlay)
        window.present();

        // Check for window resize
        if (eventHandler.wasWindowResized()) {
            int newWidth, newHeight;
            if (eventHandler.getNewWindowSize(newWidth, newHeight)) {
                std::cout << "Window resized to " << newWidth << "x" << newHeight << std::endl;
                buffer.allocate(newWidth, newHeight);
                needsRedraw = true;
            }
        }

        // Avoid busy waiting
        SDL_Delay(16); // ~60 FPS
    }

    std::cout << "Shutting down...\n";
    guiManager.shutdown();
    window.close();
    
    return 0;
}
