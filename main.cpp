//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "common.h"

#define VULKAN_INCLUDE_GLFW
#include "vulkan.h"

#include "vkutils.h"
#include "buffermanager.h"
#include "swapchainmanager.h"
#include "pipeline.h"
#include "dynamicresource.h"
#include "game.h"
#include "spritemanager.h"
#include "texture.h"
#include <glm/glm.hpp>


static inline constexpr glm::vec2 LogicalSize = { 800.0f, 600.0f };

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key >= 0)
    {
        Game* game=static_cast<Game*>(glfwGetWindowUserPointer(window));
        if (game) game->setKey(key, action != GLFW_RELEASE);
    }
}

void resize_callback(GLFWwindow* window, int width, int height)
{
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    Game* game=static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) game->updateScreenSize();
}


//!@brief
//!
//!@param argc
//!@param argv
//!@return int
int main(int argc, char* argv[])
try {

    // Step 1: initialize graphics
    // Step 1.1: initialize glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    auto window = glfwCreateWindow(LogicalSize.x, LogicalSize.y, "Break Out Volcano !!", nullptr, nullptr);

    // Step 1.2: initialize Vulkan
    vulkan.initializeInstanceGlfw(
        "Break Out Volcano",
        vk::makeVersion(1, 0, 0)
    );
 
    //! Find the proper physical and logical device
    vulkan.initializeDeviceGlfw(
        window,
        vk::ApiVersion13,
        {
            vk::KHRSwapchainExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName
        },
        vk::PhysicalDeviceFeatures2{.features = {.samplerAnisotropy = true} },   // vk::PhysicalDeviceFeatures2 
        vk::PhysicalDeviceVulkan11Features{.shaderDrawParameters = true },  // Enable shader draw parameters
        vk::PhysicalDeviceVulkan12Features{
            .shaderInt8 = true,
            .storagePushConstant8 = true
        },
        vk::PhysicalDeviceVulkan13Features{
            .dynamicRendering = true,      // Enable dynamic rendering from Vulkan 1.3
            .synchronization2 = true,       // must enable this to use sync primitives for dynamic rendering
        },
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{.extendedDynamicState = true }   // Enable extended dynamic state from the extension}
    );


   // Step 2: initialize Game
    auto breakout = make_unique<Game>(LogicalSize);
    breakout->updateScreenSize();

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, breakout.get());

    // Step 3: Run game loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        // Step 3.1: wait until we are ready for next frame (present has finished)
        if (vulkan.waitForNextFrame())
        {
            // we need a reset
            breakout->updateScreenSize();
            continue;
        }

        // Step 3.2: process input and update game state
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwPollEvents();

        breakout->processInput(deltaTime);
        breakout->update(deltaTime);
        
        // Step 3.3: render frame
        auto& commandBuffer = vulkan.beginFrame(vk::ClearColorValue(0.0f, 0.0f, 0.05f, 1.0f));

        breakout->draw(commandBuffer);

        if (vulkan.endFrame(commandBuffer))
        {
            breakout->updateScreenSize();
        }
    }

    vulkan.getDevice().waitIdle();

    glfwSetWindowUserPointer(window, nullptr);
    breakout=nullptr;

    vulkan.cleanup();
 
    // Step 4: cleanup glfw (vulkan uses RAII and doesn't need cleanup code)
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
catch (vk::SystemError& e)
{
    cerr << "Vulkan error: " << e.what() << std::endl;
    return 1;
}
catch (runtime_error& e)
{
    cerr << "runtime error: " << e.what() << std::endl;
    return 2;
}
catch (...)
{
    cerr << "unknown error" << std::endl;
    return -1;
}
