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
    if (swapChain) swapChain->cleanup();
    swapChain = nullptr;

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

    swapChain = make_unique<SwapChainManager>(
        device,
        graphicsQueue,
        presentQueue,
        graphicsQueueIndex,
        2
    );
    swapChain->reset(physicalDevice, surface);
    viewport=vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain->getExtent().width), static_cast<float>(swapChain->getExtent().height), 0.0f, 1.0f);
    renderArea=vk::Rect2D(vk::Offset2D(0, 0), swapChain->getExtent());
}

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
//    vk::ClearValue clearColor = ;
    auto attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = swapChain->getCurrentView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear
    };

    auto renderingInfo = vk::RenderingInfo{
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, renderArea);

    return commandBuffer;
}

bool Vulkan::endFrame(vk::raii::CommandBuffer& buffer)
{
    buffer.endRendering();

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

