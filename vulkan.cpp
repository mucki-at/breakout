//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "vulkan.h"

#ifdef GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "vkutils.h"

Vulkan vulkan;  // the global vulkan instance

Vulkan::Vulkan() :
    context(),
    instance(nullptr),
    surface(nullptr),
    physicalDevice(nullptr),
    device(nullptr),
    graphicsQueue(nullptr),
    presentQueue(nullptr),
    graphicsQueueIndex(-1),
    presentQueueIndex(-1)
{
}

void Vulkan::cleanup()
{
    bufferManager = nullptr;
    vmaAllocator.reset();
    presentQueue=nullptr;
    graphicsQueue=nullptr;
    device=nullptr;
    physicalDevice=nullptr;
    surface=nullptr;
    instance=nullptr;
}

void Vulkan::initializeInstance(
    const char* name,
    uint32_t version,
    vector<const char*> requiredExtensions,
    vk::InstanceCreateFlags instanceCreateFlags,
    uint32_t apiVersion
)
{
#ifdef __APPLE__
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);       // required for MAC OS
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

    requiredExtensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName); // required for MAC OS as dependency for portability subset later on
#endif

    auto appInfo=vk::ApplicationInfo{
        .pApplicationName = name,
        .applicationVersion = version,
        .apiVersion = apiVersion
    };

    //! Create a vulkan instance (global object to interact with vulkan api)
    instance = vk::raii::Instance(
        context,
        vk::InstanceCreateInfo {
            .flags=instanceCreateFlags,
            .pApplicationInfo = &appInfo
        }.setPEnabledExtensionNames(requiredExtensions)
    );
}

void Vulkan::initializeDeviceInternal
(
    uint32_t apiVersion,
    vector<const char*> deviceExtensions,
    const void* featuresHead
)
{
#ifdef __APPLE__
    deviceExtensions.push_back(vk::KHRPortabilitySubsetExtensionName);
#endif

    tie(physicalDevice, graphicsQueueIndex, presentQueueIndex) =
    findAppropriateDeviceAndQueueFamily(
        instance,
        {
            .apiVersion=apiVersion,
            .deviceExtensions = deviceExtensions,
            .surface = surface
        }
    );

    float prio = 1.0f;
    auto queueInfo = vk::DeviceQueueCreateInfo
    {
        .queueFamilyIndex = graphicsQueueIndex
    }.setQueuePriorities(prio);
    // TODO: why do we not need a second queueInfo for presentQueue if the index is different???

    auto createInfo = vk::DeviceCreateInfo
    {
        .pNext = featuresHead
    };
    createInfo.setQueueCreateInfos(queueInfo);
    createInfo.setPEnabledExtensionNames(deviceExtensions);

    device = vk::raii::Device(physicalDevice, createInfo);

    //! Create graphics and present queues
    graphicsQueue = vk::raii::Queue(device, graphicsQueueIndex, 0);
    presentQueue = vk::raii::Queue(device, presentQueueIndex, 0);

    vmaAllocator = vma::createAllocatorUnique({
        .physicalDevice=physicalDevice,
        .device=device,
        .instance=instance
    });
    
    bufferManager = make_unique<BufferManager>(*vmaAllocator, device, graphicsQueue, graphicsQueueIndex);

    //! pick a color format for our swap chain
    auto availableFormats=physicalDevice.getSurfaceFormatsKHR(surface);
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            swapChainFormat=availableFormat;
            break;
        }
    }
    if (swapChainFormat.format==vk::Format::eUndefined)
    {
        swapChainFormat=availableFormats[0];
    }
}

#if 0
bool Vulkan::waitForNextFrame()
{
    if (swapChain->waitForNextFrame())
    {
        // we need a reset
        swapChain->reset(physicalDevice, surface);
        viewport=vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain->getExtent().width), static_cast<float>(swapChain->getExtent().height), 0.0f, 1.0f);
        renderArea=vk::Rect2D(vk::Offset2D(0, 0), swapChain->getExtent());
        
        return true;
    }
    return false;
}

vk::raii::CommandBuffer& Vulkan::beginFrame(const vk::ClearValue& clear)
{
    auto& commandBuffer = swapChain->beginFrame();
    swapChain->beginRenderTo(commandBuffer, clear);
    return commandBuffer;
}

bool Vulkan::endFrame(vk::raii::CommandBuffer& buffer)
{
    swapChain->endRenderTo(buffer);

    if (swapChain->endFrame())
    {
        // we need a reset
        swapChain->reset(physicalDevice, surface);
        viewport=vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain->getExtent().width), static_cast<float>(swapChain->getExtent().height), 0.0f, 1.0f);
        renderArea=vk::Rect2D(vk::Offset2D(0, 0), swapChain->getExtent());
        
        return true;
    }
    return false;
 
}
#endif