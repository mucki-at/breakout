//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "rendertarget.h"
#include "vkutils.h"

class SwapChain : public RenderTarget
{
public:
    SwapChain(
        uint32_t maxFramesInFlight=2
    );

    void cleanup();
    void reset(
        const SwapChainRequirements& requirements={}
    );

    bool waitForNextFrame();
    vk::raii::CommandBuffer& beginFrame();

    bool endFrame(const vk::CommandBuffer& commandBuffer);

private:
    vk::raii::CommandPool commandPool;
    vk::raii::CommandBuffers commandBuffers;
    vector<vk::raii::Semaphore> presentCompleteSemaphores;
    vector<vk::raii::Semaphore> renderFinishedSemaphores;
    vector<vk::raii::Fence> inFlightFences;

    size_t currentFrame;
    uint32_t currentImage;
    vk::raii::SwapchainKHR chain = nullptr;
};

