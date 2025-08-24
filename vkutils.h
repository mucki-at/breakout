//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once
#include "common.h"

struct DeviceRequirements
{
    uint32_t apiVersion = vk::ApiVersion10;
    vk::PhysicalDeviceFeatures features = {};
    vk::QueueFlags queueFlags = vk::QueueFlagBits::eGraphics;
    std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };
    const vk::raii::SurfaceKHR* surface = nullptr;
};

using DeviceScore=function<float(const vk::PhysicalDeviceProperties&, const vk::PhysicalDeviceFeatures&)>;

[[nodiscard]] tuple<vk::raii::PhysicalDevice, uint32_t, uint32_t>
findAppropriateDeviceAndQueueFamily(
    const vk::raii::Instance& instance,
    const DeviceRequirements& requirements={},
    DeviceScore score=[](const vk::PhysicalDeviceProperties& dp, const vk::PhysicalDeviceFeatures& df){ return dp.deviceType==vk::PhysicalDeviceType::eDiscreteGpu ? 1.0 : 0.0; }
);

struct SwapChainRequirements
{
    vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
    vk::Extent2D fallbackSwapchainSize = {};
    uint32_t minImageCount = 2;
    optional<pair<uint32_t,uint32_t>> queueIndices={};
};

[[nodiscard]] tuple<vk::raii::SwapchainKHR, vk::Format, vk::Extent2D>
createSwapChain(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::raii::SurfaceKHR& surface,
    const SwapChainRequirements& requirements={}
);

[[nodiscard]] vk::raii::ShaderModule loadShaderModule(
    const vk::raii::Device& device,
    const std::string& filename
);

class SwapChainManager
{
public:
    SwapChainManager(
        const vk::raii::Device& device,
        const vk::raii::Queue& graphicsQueue,
        const vk::raii::Queue& presentQueue,
        uint32_t graphicsQueueFamily,
        uint32_t maxFramesInFlight=2
    );

    void cleanup();
    void reset(
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::SurfaceKHR& surface,
        const SwapChainRequirements& requirements={}
    );

    bool waitForNextFrame();
    vk::raii::CommandBuffer& beginFrame();
    bool endFrame();

    inline const auto& getFormat() const noexcept { return format; }
    inline const auto& getExtent() const noexcept { return extent; }
    inline const vk::raii::ImageView& getCurrentView() const noexcept { return views[currentImage]; }

private:
    const vk::raii::Device& device;
    const vk::raii::Queue& graphicsQueue;
    const vk::raii::Queue& presentQueue;

    vk::raii::CommandPool commandPool;
    vk::raii::CommandBuffers commandBuffers;
    vector<vk::raii::Semaphore> presentCompleteSemaphores;
    vector<vk::raii::Semaphore> renderFinishedSemaphores;
    vector<vk::raii::Fence> inFlightFences;
    size_t currentFrame;
    uint32_t currentImage;

    vk::raii::SwapchainKHR chain = nullptr;
    vk::Format format = {};
    vk::Extent2D extent = {};
    vector<vk::Image> images = {};
    vector<vk::raii::ImageView> views = {};

};