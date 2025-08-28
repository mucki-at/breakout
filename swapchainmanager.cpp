//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "swapchainmanager.h"

SwapChainManager::SwapChainManager(
    const vk::raii::Device& device,
    const vk::raii::Queue& graphicsQueue,
    const vk::raii::Queue& presentQueue,
    uint32_t graphicsQueueFamily,
    uint32_t maxFramesInFlight
) :
    device(device),
    graphicsQueue(graphicsQueue),
    presentQueue(presentQueue),
    commandPool(device, vk::CommandPoolCreateInfo
    {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamily
    }),
    commandBuffers(device, vk::CommandBufferAllocateInfo 
    {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = maxFramesInFlight
    }),
    presentCompleteSemaphores(),
    renderFinishedSemaphores(),
    inFlightFences(),
    currentFrame(0)
{
    for (auto i=0; i<maxFramesInFlight; ++i)
    {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void SwapChainManager::cleanup()
{
    device.waitIdle();
    views.clear();
    images.clear();
    chain = nullptr;
}

void SwapChainManager::reset(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::SurfaceKHR& surface,
    const SwapChainRequirements& requirements
)
{
    cleanup();
    
    tie(chain, format, extent) = createSwapChain(physicalDevice, device, surface, requirements);
    images = chain.getImages();
    views.clear();
    for (auto&& image: images)
    {
        views.emplace_back(device, vk::ImageViewCreateInfo{
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .image=image
        });
    }
}

bool SwapChainManager::waitForNextFrame()
{
    while ( vk::Result::eTimeout == device.waitForFences( *inFlightFences[currentFrame], vk::True, UINT64_MAX ) )
        ;

    vk::Result result;
    tie(result, currentImage) = chain.acquireNextImage( UINT64_MAX, presentCompleteSemaphores[currentFrame], nullptr );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        return true;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    return false;
}

vk::raii::CommandBuffer& SwapChainManager::beginFrame()
{
    device.resetFences( *inFlightFences[currentFrame] );

    vk::raii::CommandBuffer& commandBuffer = commandBuffers[currentFrame];
    commandBuffer.reset();

    commandBuffer.begin({});
    auto barrier = vk::ImageMemoryBarrier2
    {
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = images[currentImage],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    commandBuffer.pipelineBarrier2(dependencyInfo);

    return commandBuffer;
}

bool SwapChainManager::endFrame()
{
    vk::raii::CommandBuffer& commandBuffer = commandBuffers[currentFrame];

    auto barrier = vk::ImageMemoryBarrier2
    {
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = images[currentImage],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    commandBuffer.pipelineBarrier2(dependencyInfo);
    commandBuffer.end();

    auto waitDestinationStageMask = vk::PipelineStageFlags( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    auto submitInfo=vk::SubmitInfo{};
    submitInfo.setCommandBuffers(*commandBuffer);
    submitInfo.setWaitSemaphores(*presentCompleteSemaphores[currentFrame]);
    submitInfo.setWaitDstStageMask(waitDestinationStageMask);
    submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

    graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

    auto presentInfo=vk::PresentInfoKHR{};
    presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);
    presentInfo.setSwapchains(*chain);
    presentInfo.setImageIndices(currentImage);
    auto result = presentQueue.presentKHR(presentInfo);

    currentFrame=(currentFrame+1)%commandBuffers.size();

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        return true;        
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    return false;
}
