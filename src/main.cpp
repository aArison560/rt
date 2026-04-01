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
#include "Scene.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "EventHandler.hpp"
#include "ImageBuffer.hpp"
#include "SceneParser.hpp"
#include "Sphere.hpp"
#include "Plane.hpp"
#include "PointLight.hpp"
#include "AmbientLight.hpp"

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

    // TODO: Create a simple default scene with:
    // - A sphere
    // - A plane (ground)
    // - Point light
    // - Ambient light
    // - Camera

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
