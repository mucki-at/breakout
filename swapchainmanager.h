//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "vkutils.h"
#include "buffermanager.h"

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

