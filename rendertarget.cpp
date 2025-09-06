//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "rendertarget.h"
#include "buffermanager.h"
#include "vulkan.h"

RenderTarget::RenderTarget() :
    images(),
    current(images.begin())
{
}

void RenderTarget::beginRenderTo(const vk::CommandBuffer& commandBuffer, const vk::ClearValue& clear)
{
    current->discardAndTransition(commandBuffer, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal);
    
    auto attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = *current,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear
    };

    auto renderArea=vk::Rect2D(vk::Offset2D(0, 0), description.extent);
    auto renderingInfo = vk::RenderingInfo{
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(description.extent.width), static_cast<float>(description.extent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, renderArea);
}

void RenderTarget::endRenderTo(const vk::CommandBuffer& commandBuffer)
{
    commandBuffer.endRendering();
}
