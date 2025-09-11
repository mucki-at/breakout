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

void RenderTarget::createImages(
    const ImageDescription& description,
    vk::ImageUsageFlags usage,
    size_t imageCount
)
{
    this->description=description,
    this->usage=usage;

    vulkan.getDevice().waitIdle();
    images.clear();
    while (imageCount--)
    {
        images.push_back(vulkan.getBufferManager().createImage(
            description,
            usage,
            vk::SampleCountFlagBits::e1
        ));
    }
    current=images.begin();

}

void RenderTarget::beginRenderTo(const vk::CommandBuffer& commandBuffer, const vk::ClearValue& clear)
{
    current->discardAndTransition(commandBuffer, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal);
    
    auto renderArea=vk::Rect2D(vk::Offset2D(0, 0), description.extent);
    auto attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = *current,
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
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(description.extent.width), static_cast<float>(description.extent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, renderArea);
}

void RenderTarget::endRenderTo(const vk::CommandBuffer& commandBuffer)
{
    commandBuffer.endRendering();
}

void RenderTarget::cycle()
{
    ++current;
    if (current==images.end()) current=images.begin();
}


MultisampleRenderTarget::MultisampleRenderTarget() :
    RenderTarget(),
    msImages()
{
}

void MultisampleRenderTarget::createImages(
    const ImageDescription& description,
    vk::ImageUsageFlags usage,
    vk::SampleCountFlagBits samples,
    size_t imageCount
)
{
    RenderTarget::createImages(description, usage, imageCount);

    this->samples=samples;
    msImages.clear();
    for (auto&& img : images)
    {
        msImages.push_back(vulkan.getBufferManager().createImage(
            description,
            vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
            samples
        ));
    }
}

void MultisampleRenderTarget::beginRenderTo(const vk::CommandBuffer& commandBuffer, const vk::ClearValue& clear)
{
    auto curMs=msImages.begin() + (current-images.begin());

    current->discardAndTransition(commandBuffer, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal);
    curMs->discardAndTransition(commandBuffer, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal);
    
    auto renderArea=vk::Rect2D(vk::Offset2D(0, 0), description.extent);
    auto attachmentInfo = vk::RenderingAttachmentInfo{
        .imageView = *curMs,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveImageView = *current,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .clearValue = clear
    };

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
