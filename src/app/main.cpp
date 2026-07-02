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
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include "scene/Scene.hpp"
#include "scene/Camera.hpp"
#include "rendering/Renderer.hpp"
#include "platform/Window.hpp"
#include "platform/EventHandler.hpp"
#include "rendering/ImageBuffer.hpp"
#include "scene/SceneParser.hpp"
#include "geometry/Sphere.hpp"
#include "geometry/Plane.hpp"
#include "lighting/PointLight.hpp"
#include "lighting/AmbientLight.hpp"

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
    scene.setBackgroundColor(Vec3(0.2, 0.2, 0.3));

    // Simple default scene: one sphere, ambient + point light, camera
    scene.setName("Default Scene");

    // Camera
    Camera cam(Vec3(0.0, 1.5, 8.0), Vec3(0.0, 0.0, -1.0), Vec3(0.0, 1.0, 0.0), 50.0);
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
    std::cout << "Usage: " << programName << " [scene_file] [width] [height]\n"
              << "  scene_file: Path to .rt scene file (default: " << DEFAULT_SCENE_FILE << ")\n"
              << "  width:      Output width in pixels (default: " << DEFAULT_WIDTH << ")\n"
              << "  height:     Output height in pixels (default: " << DEFAULT_HEIGHT << ")\n"
              << "\nExample: " << programName << " scenes/test.rt 1920 1080\n";
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[])
{
    // TODO: Parse command line arguments
    std::string sceneFile = DEFAULT_SCENE_FILE;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;

    // Parse arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        sceneFile = argv[1];
    }
    
    if (argc > 2) {
        try {
            width = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid width: " << argv[2] << std::endl;
            return 1;
        }
    }
    
    if (argc > 3) {
        try {
            height = std::stoi(argv[3]);
        } catch (...) {
            std::cerr << "Invalid height: " << argv[3] << std::endl;
            return 1;
        }
    }

    // TODO: Load scene from file or create default
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

    // TODO: Initialize window
    Window window("RT - Ray Tracer", width, height);
    if (!window.initialize("RT - Ray Tracer", width, height)) {
        std::cerr << "Failed to initialize window\n";
        return 1;
    }
    std::cout << "Window initialized: " << width << "x" << height << std::endl;

    // TODO: Setup rendering
    Renderer renderer;
    renderer.setMaxRecursionDepth(4);
    renderer.setShadowsEnabled(true);
    renderer.setReflectionsEnabled(true);

    ImageBuffer buffer(width, height);

    // Quick debug fill to verify the window shows image data
    buffer.fill(255, 0, 255, 255); // magenta test
    if (!window.updateDisplay(buffer)) {
        std::cerr << "Warning: initial updateDisplay failed\n";
    } else {
        window.present();
    }

    // Pause briefly so user can see the test pattern (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // TODO: Setup event handler
    EventHandler eventHandler;
    eventHandler.setupCameraControls(&scene.getCamera(), 0.5, 0.05);
    eventHandler.onExpose([&]() {
        // TODO: Handle expose event (partial redraw)
    });

    std::cout << "Starting render loop...\n";

    // TODO: Main render loop
    bool running = true;
    bool needsRedraw = true;

    while (running) {
        // Process events
        if (eventHandler.pollEvents()) {
            running = false;
            break;
        }

        // Check for redraw
        if (needsRedraw) {
            // TODO: Render scene
            std::cout << "Rendering scene...\n";
            if (!renderer.render(scene, width, height, buffer.getData())) {
                std::cerr << "Render failed\n";
                running = false;
                break;
            }

            // TODO: Display rendered image
            if (!window.updateDisplay(buffer)) {
                std::cerr << "Failed to update display\n";
                running = false;
                break;
            }
            window.present();
            needsRedraw = false;
        }

        // TODO: Check for window resize
        if (eventHandler.wasWindowResized()) {
            int newWidth, newHeight;
            if (eventHandler.getNewWindowSize(newWidth, newHeight)) {
                std::cout << "Window resized to " << newWidth << "x" << newHeight << std::endl;
                buffer.allocate(newWidth, newHeight);
                needsRedraw = true;
            }
        }

        // TODO: Sleep briefly to avoid busy waiting
        // SDL_Delay(16); // ~60 FPS
    }

    std::cout << "Shutting down...\n";
    window.close();
    
    return 0;
}
