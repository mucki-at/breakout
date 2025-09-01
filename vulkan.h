//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "vma.h"
#include "buffermanager.h"
#include "swapchainmanager.h"

#ifdef VULKAN_INCLUDE_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#ifdef VULKAN_INCLUDE_SDL3
#include <SDL3/SDL_vulkan.h>
#endif

class Vulkan
{
public:
    Vulkan();

    void cleanup();

    void initializeInstance(
        const char* name,
        uint32_t version,
        vector<const char*> requiredExtensions,
        vk::InstanceCreateFlags instanceCreateFlags={},
        uint32_t apiVersion=vk::ApiVersion14
    );

#ifdef VULKAN_INCLUDE_GLFW
    void initializeInstanceGlfw(
        const char* name,
        uint32_t version,
        vector<const char*> requiredExtensions={},
        vk::InstanceCreateFlags instanceCreateFlags={},
        uint32_t apiVersion=vk::ApiVersion14
    )
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        copy(glfwExtensions, glfwExtensions+glfwExtensionCount, back_inserter(requiredExtensions));
        return initializeInstance(name, version, requiredExtensions, instanceCreateFlags, apiVersion);
    }
#endif

#ifdef VULKAN_INCLUDE_SDL3
    void initializeInstanceSDL3(
        const char* name,
        uint32_t version,
        vector<const char*> requiredExtensions={},
        vk::InstanceCreateFlags instanceCreateFlags={},
        uint32_t apiVersion=vk::ApiVersion14
    )
    {
        Uint32 sdlExtensionCount=0;
        auto sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        if (sdlExtensions==nullptr) throw runtime_error("failed to get SDL instance extensions: "s+SDL_GetError());

        copy(sdlExtensions, sdlExtensions+sdlExtensionCount, back_inserter(requiredExtensions));
        return initializeInstance(name, version, requiredExtensions, instanceCreateFlags, apiVersion);
    }
#endif


    template<typename... Features>
    inline void initializeDevice
    (
        uint32_t apiVersion,
        const vector<const char*>& deviceExtensions,
        Features&&... features
    )
    {
        vk::StructureChain<Features...> featureChain=
        {
            std::forward<Features>(features)...
        };

        initializeDeviceInternal(
            apiVersion,
            deviceExtensions,
            get<0>(featureChain)
        );
    }
#ifdef VULKAN_INCLUDE_GLFW
    template<typename... Features>
    void initializeDeviceGlfw
    (
        GLFWwindow* window,
        uint32_t apiVersion,
        const vector<const char*>& deviceExtensions,
        Features&&... features
    )
    {
        VkSurfaceKHR surfaceHandle;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &surfaceHandle) != 0)
            throw std::runtime_error("failed to create window surface!");
    
        surface=vk::raii::SurfaceKHR(instance, surfaceHandle); // make RAII type from SDL created

        return initializeDevice<Features...>(apiVersion, deviceExtensions, std::forward<Features>(features)...);
    }
#endif

#ifdef VULKAN_INCLUDE_SDL3
    template<typename... Features>
    void initializeDeviceSDL3
    (
        SDL_Window* window,
        uint32_t apiVersion,
        const vector<const char*>& deviceExtensions,
        Features&&... features
    )
    {
        VkSurfaceKHR surfaceHandle;
        if (!SDL_Vulkan_CreateSurface(window, *instance, nullptr, &surfaceHandle))
            throw std::runtime_error("failed to create window surface: "s+SDL_GetError());
    
        surface=vk::raii::SurfaceKHR(instance, surfaceHandle); // make RAII type from SDL created

        return initializeDevice<Features...>(apiVersion, deviceExtensions, std::forward<Features>(features)...);
    }
#endif

    bool waitForNextFrame();
    vk::raii::CommandBuffer& beginFrame(const vk::ClearValue& clear);
    bool endFrame(vk::raii::CommandBuffer& buffer);

private:
    void initializeDeviceInternal
    (
        uint32_t apiVersion,
        vector<const char*> deviceExtensions,
        const void* featuresHead
    );

public:
    inline const vk::raii::Instance& getInstance() const noexcept { return instance; }
    inline const vk::raii::SurfaceKHR& getSurface() const noexcept { return surface; }
    inline const vk::raii::PhysicalDevice& getPhysicalDevice() const noexcept { return physicalDevice; }
    inline const vk::raii::Device& getDevice() const noexcept { return device; }
    inline const vk::raii::Queue& getGraphicsQueue() const noexcept { return graphicsQueue; }
    inline const vk::raii::Queue& getPresentQueue() const noexcept { return presentQueue; }
    inline uint32_t getGraphicsQueueIndex() const noexcept { return graphicsQueueIndex; }
    inline uint32_t getPresentQueueIndex() const noexcept { return presentQueueIndex; }

    inline const vma::UniqueAllocator& getVmaAllocator() const noexcept { return vmaAllocator; }
    inline const BufferManager& getBufferManager() const noexcept { return *bufferManager; }
    inline const SwapChainManager& getSwapChain() const noexcept { return *swapChain; }

    inline const vk::Rect2D& getRenderArea() const noexcept { return renderArea; }
    inline const vk::Viewport& getViewport() const noexcept { return viewport; }

private:
    vk::raii::Context context;
    vk::raii::Instance instance;
    vk::raii::SurfaceKHR surface;
    vk::raii::PhysicalDevice physicalDevice;
    vk::raii::Device device;
    vk::raii::Queue graphicsQueue;
    vk::raii::Queue presentQueue;
    uint32_t graphicsQueueIndex;
    uint32_t presentQueueIndex;

    vma::UniqueAllocator vmaAllocator;
    unique_ptr<BufferManager> bufferManager;

    unique_ptr<SwapChainManager> swapChain;

    vk::Rect2D renderArea;
    vk::Viewport viewport;
};

extern Vulkan vulkan;