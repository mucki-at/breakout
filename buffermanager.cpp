//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "buffermanager.h"
#include "vkutils.h"
#include "vulkan.h"

/////// DeviceBuffer

DeviceBuffer::DeviceBuffer(
    vma::Allocator allocator,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    const vma::AllocationCreateInfo& allocInfo
    ) :
    allocator(allocator),
    buffer(nullptr),
    allocation(nullptr)
{
    tie(buffer, allocation) = allocator.createBuffer({.size=size, .usage=usage}, allocInfo, &info);
}

DeviceBuffer::DeviceBuffer(DeviceBuffer&& rhs) :
    allocator(exchange(rhs.allocator, {})),
    buffer(exchange(rhs.buffer, {})),
    allocation(exchange(rhs.allocation, {})),
    info(exchange(rhs.info, {}))
{
}

DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& rhs)
{
    if (this!=&rhs)
    {
        swap(allocator, rhs.allocator);
        swap(buffer, rhs.buffer);
        swap(allocation, rhs.allocation);
        swap(info, rhs.info);
    }
    return *this;
}

DeviceBuffer::~DeviceBuffer()
{
    if (allocator) allocator.destroyBuffer(buffer, allocation);
}

/////// DeviceImage

// this constructor is used by buffermanager to allocate new images
DeviceImage::DeviceImage(
    vma::Allocator allocator,
    vk::Device device,
    const ImageDescription& description,
    vk::ImageUsageFlags usage,
    vk::SampleCountFlagBits samples,
    const vma::AllocationCreateInfo& allocInfo
    ) :
    description(description),
    allocator(allocator),
    image(nullptr),
    view(nullptr),
    allocation(nullptr),
    currentStage(vk::PipelineStageFlagBits2::eNone),
    currentAccess(vk::AccessFlagBits2::eNone),
    currentLayout(vk::ImageLayout::eUndefined)
{
    tie(image, allocation) = allocator.createImage(
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = description.format,
            .extent = { description.extent.width, description.extent.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = samples,
            .tiling = vk::ImageTiling::eOptimal,
            .usage=usage,
            .sharingMode=vk::SharingMode::eExclusive,
            .initialLayout=vk::ImageLayout::eUndefined
        },
        allocInfo,
        &info
    );
    
    view = device.createImageView({
        .image=image,
        .viewType=vk::ImageViewType::e2D,
        .format=description.format,
        .subresourceRange={vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    });
}

// this constructor is used by SwapChain to hold images and views for the swapchain.
// the image storage is managed by the vulkan swap chain object, and so we do not
// need to allocate or free them, but we need to create the view for it
DeviceImage::DeviceImage(
    const ImageDescription& description,
    vk::Image image
) :
    description(description),
    allocator(nullptr),
    image(image),
    view(nullptr),
    allocation(nullptr),
    currentStage(vk::PipelineStageFlagBits2::eNone),
    currentAccess(vk::AccessFlagBits2::eNone),
    currentLayout(vk::ImageLayout::eUndefined)   
{
    // we explicitelydo NOT want the raii version here.
    vk::Device deviceHandle=vulkan.getDevice();
    view=deviceHandle.createImageView({
        .viewType = vk::ImageViewType::e2D,
        .format = description.format,
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


DeviceImage::DeviceImage(DeviceImage&& rhs) :
    description(exchange(rhs.description, {})),
    allocator(exchange(rhs.allocator, {})),
    image(exchange(rhs.image, {})),
    view(exchange(rhs.view, {})),
    allocation(exchange(rhs.allocation, {})),
    info(exchange(rhs.info, {})),
    currentStage(exchange(rhs.currentStage, {})),
    currentAccess(exchange(rhs.currentAccess, {})),
    currentLayout(exchange(rhs.currentLayout, {}))
{
}

DeviceImage& DeviceImage::operator=(DeviceImage&& rhs)
{
    if (this!=&rhs)
    {
        swap(description, rhs.description);
        swap(allocator, rhs.allocator);
        swap(image, rhs.image);
        swap(view, rhs.view);
        swap(allocation, rhs.allocation);
        swap(info, rhs.info);
        swap(currentStage, rhs.currentStage);
        swap(currentAccess, rhs.currentAccess);
        swap(currentLayout, rhs.currentLayout);
    }
    return *this;
}


DeviceImage::~DeviceImage()
{
    if (allocator)
    {
        auto info=allocator.getAllocatorInfo();
        info.device.destroyImageView(view);
        allocator.destroyImage(image, allocation);
    }
    else
    {
        if (view)
        {
            vk::Device deviceHandle=vulkan.getDevice();
            deviceHandle.destroyImageView(view);
        }
    }
}

void DeviceImage::createBarrier(
    const vk::CommandBuffer& commandBuffer,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::ImageLayout srcLayout,
    vk::ImageLayout dstLayout
)
{
    auto barrier = vk::ImageMemoryBarrier2
    {
        .srcStageMask = srcStageMask,
        .dstStageMask = dstStageMask,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = srcLayout,
        .newLayout = dstLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
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
    currentStage = dstStageMask;
    currentAccess = dstAccessMask;
    currentLayout = dstLayout;

}


/////// BufferManager

BufferManager::BufferManager(
    const vma::Allocator& allocator,
    const vk::raii::Device& device,
    const vk::raii::Queue& transferQueue,
    uint32_t transferQueueFamily,
    vk::DeviceSize initialStagingSize
) :
    allocator(allocator),
    device(device),
    transferQueue(transferQueue),
    commandPool(device, vk::CommandPoolCreateInfo{
        .queueFamilyIndex=transferQueueFamily,
        .flags = vk::CommandPoolCreateFlagBits::eTransient
    }),
    stagingBuffer(createBuffer(1024*1024, vk::BufferUsageFlagBits::eTransferSrc, true)),
    fence(device, vk::FenceCreateInfo{})
{
}

void BufferManager::resizeStage(vk::DeviceSize minSize) const
{
    stagingBuffer = createBuffer(minSize, vk::BufferUsageFlagBits::eTransferSrc, true);
}

void BufferManager::upload(const vk::Buffer& buffer, const vk::BufferCopy& range) const
{
    auto commands=device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    auto& copy=commands.front();
    copy.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    copy.copyBuffer(stagingBuffer, buffer, range);
    copy.end();
    auto submitInfo=vk::SubmitInfo{
        .commandBufferCount=1,
        .pCommandBuffers=&*copy
    };
    transferQueue.submit(submitInfo, fence);
    if (device.waitForFences({fence}, true, numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
        throw std::runtime_error("Host to device buffer transfer timed out");
    device.resetFences({fence});
}

void BufferManager::upload(DeviceImage& image, const vk::BufferImageCopy& region) const
{
    auto commands=device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    auto& copy=commands.front();
    copy.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    image.discardAndTransition(copy, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal);
    copy.copyBufferToImage(stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    image.transition(copy, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead, vk::ImageLayout::eShaderReadOnlyOptimal);
    copy.end();
    auto submitInfo=vk::SubmitInfo{
        .commandBufferCount=1,
        .pCommandBuffers=&*copy
    };
    transferQueue.submit(submitInfo, fence);
    if (device.waitForFences({fence}, true, numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
        throw std::runtime_error("Host to device image transfer timed out");
    device.resetFences({fence});
}


