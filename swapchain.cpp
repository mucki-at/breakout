//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "swapchain.h"
#include "vulkan.h"

SwapChain::SwapChain(uint32_t maxFramesInFlight) :
    RenderTarget(),
    commandPool(vulkan.getDevice(), vk::CommandPoolCreateInfo
    {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vulkan.getGraphicsQueueIndex()
    }),
    commandBuffers(vulkan.getDevice(), vk::CommandBufferAllocateInfo 
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
        presentCompleteSemaphores.emplace_back(vulkan.getDevice(), vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(vulkan.getDevice(), vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(vulkan.getDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void SwapChain::cleanup()
{
    vulkan.getDevice().waitIdle();
    images.clear();
    chain = nullptr;
}

void SwapChain::reset(
   const SwapChainRequirements& requirements
)
{
    cleanup();
    
    tie(chain, description.format, description.extent) = createSwapChain(vulkan.getPhysicalDevice(), vulkan.getDevice(), vulkan.getSurface(), vulkan.getSwapChainFormat(), requirements);
    for (auto&& image: chain.getImages()) images.emplace_back(DeviceImage(description, image));
    current=images.begin();
}

bool SwapChain::waitForNextFrame()
{
    while ( vk::Result::eTimeout == vulkan.getDevice().waitForFences( *inFlightFences[currentFrame], vk::True, UINT64_MAX ) )
        ;

    vk::Result result;
    tie(result, currentImage) = chain.acquireNextImage( UINT64_MAX, presentCompleteSemaphores[currentFrame], nullptr );
    current=images.begin()+currentImage;

    if (result == vk::Result::eErrorOutOfDateKHR) {
        return true;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    return false;
}

vk::raii::CommandBuffer& SwapChain::beginFrame()
{
    vulkan.getDevice().resetFences( *inFlightFences[currentFrame] );

    vk::raii::CommandBuffer& commandBuffer = commandBuffers[currentFrame];
    commandBuffer.reset();
    commandBuffer.begin({});

    return commandBuffer;
}

bool SwapChain::endFrame(const vk::CommandBuffer& commandBuffer)
{
    current->transition(commandBuffer, vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR);
    commandBuffer.end();

    auto waitDestinationStageMask = vk::PipelineStageFlags( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    auto submitInfo=vk::SubmitInfo{};
    submitInfo.setCommandBuffers(commandBuffer);
    submitInfo.setWaitSemaphores(*presentCompleteSemaphores[currentFrame]);
    submitInfo.setWaitDstStageMask(waitDestinationStageMask);
    submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

    vulkan.getGraphicsQueue().submit(submitInfo, *inFlightFences[currentFrame]);

    auto presentInfo=vk::PresentInfoKHR{};
    presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);
    presentInfo.setSwapchains(*chain);
    presentInfo.setImageIndices(currentImage);
    auto result = vulkan.getPresentQueue().presentKHR(presentInfo);

    currentFrame=(currentFrame+1)%commandBuffers.size();

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        return true;        
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    return false;
}
