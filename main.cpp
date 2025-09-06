//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "common.h"

#define VULKAN_INCLUDE_SDL3
#include "vulkan.h"

#include "vkutils.h"
#include "buffermanager.h"
#include "swapchain.h"
#include "dynamicresource.h"
#include "game.h"
#include "spritemanager.h"
#include "texture.h"
#include <glm/glm.hpp>

#include <SDL3/SDL.h>
#include <chrono>

static inline constexpr glm::vec2 LogicalSize = { 800.0f, 600.0f };

using GameClock = chrono::high_resolution_clock;
using Seconds = chrono::duration<float>;

//!@brief
//!
//!@param argc
//!@param argv
//!@return int
int main(int argc, char* argv[])
try {
    // Step 1: initialize graphics
    // Step 1.1: initialize SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    auto window = SDL_CreateWindow(
        "Break Out Volcano !!",
        LogicalSize.x,
        LogicalSize.y,
        SDL_WINDOW_RESIZABLE | 
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_VULKAN
    );

    // Step 1.2: initialize Vulkan
    vulkan.initializeInstanceSDL3(
        "Break Out Volcano",
        vk::makeVersion(1, 0, 0)
    );
 
    //! Find the proper physical and logical device
    vulkan.initializeDeviceSDL3(
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

    auto swapChain=make_unique<SwapChain>(2);
    swapChain->reset();

    // Step 2: initialize Game
    auto breakout = make_unique<Game>("levels",LogicalSize);
    breakout->updateScreenSize(swapChain->getDescription().extent);

    // Step 3: Run game loop
    auto lastFrame=GameClock::now();
    bool done=false;
    SDL_Event event;
    bool paused=false;

    while (!done)
    {
        // Step 3.1: wait until we are ready for next frame (present has finished)
        if (swapChain->waitForNextFrame())
        {
            // we need a reset
            swapChain->reset();
            breakout->updateScreenSize(swapChain->getDescription().extent);
            continue;
        }

        bool restartLoop=false;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_WINDOW_RESIZED:
                swapChain->reset();
                breakout->updateScreenSize(swapChain->getDescription().extent);
                restartLoop=true;
                break;

            case SDL_EVENT_QUIT:
            case SDL_EVENT_TERMINATING:
                done=true; break;

            case SDL_EVENT_WILL_ENTER_BACKGROUND:
            case SDL_EVENT_WINDOW_HIDDEN:
            case SDL_EVENT_WINDOW_MINIMIZED:
                paused=true; break;

            case SDL_EVENT_DID_ENTER_FOREGROUND:
            case SDL_EVENT_WINDOW_MAXIMIZED:         /**< Window has been maximized */
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_SHOWN:
                if (paused)
                {
                    lastFrame=GameClock::now();
                    paused=false;
                }
                break;
                

            case SDL_EVENT_KEY_DOWN:
                breakout->setKey(event.key.scancode, true);
                if (event.key.scancode==SDL_SCANCODE_ESCAPE) done=true;
                break;

            case SDL_EVENT_KEY_UP:
                breakout->setKey(event.key.scancode, false);

            default: break;
            }
        }

        if (restartLoop)
        {
            continue;
        }
        
        if (paused)
        {
            SDL_WaitEventTimeout(nullptr, 250);
            continue;
        }

        // Step 3.2: process input and update game state
        auto currentFrame = GameClock::now();
        auto deltaTime = chrono::duration_cast<Seconds>(currentFrame-lastFrame);
        lastFrame = currentFrame;

        breakout->processInput(deltaTime.count());
        breakout->update(deltaTime.count());
        
        // Step 3.3: render frame
        auto& commandBuffer = swapChain->beginFrame();
        
        swapChain->beginRenderTo(commandBuffer, vk::ClearColorValue(0.0f, 0.0f, 0.05f, 1.0f));
        breakout->draw(commandBuffer);
        swapChain->endRenderTo(commandBuffer);

        if (swapChain->endFrame(commandBuffer))
        {
            swapChain->reset();
            breakout->updateScreenSize(swapChain->getDescription().extent);
        }
    }

    vulkan.getDevice().waitIdle();

    breakout=nullptr;
    swapChain=nullptr;
    
    vulkan.cleanup();
 
    // Step 4: cleanup SDL (vulkan uses RAII and doesn't need cleanup code)
    SDL_DestroyWindow(window);
    SDL_Quit();
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
