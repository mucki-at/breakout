//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "buffermanager.h"
#include "vkutils.h"

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

DeviceImage::DeviceImage(
    vma::Allocator allocator,
    vk::Device device,
    const ImageDescription& description,
    vk::ImageUsageFlags usage,
    const vma::AllocationCreateInfo& allocInfo
    ) :
    allocator(allocator),
    image(nullptr),
    view(nullptr),
    allocation(nullptr)
{
    tie(image, allocation) = allocator.createImage(
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = description.format,
            .extent = { description.width, description.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
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

DeviceImage::DeviceImage(DeviceImage&& rhs) :
    allocator(exchange(rhs.allocator, {})),
    image(exchange(rhs.image, {})),
    view(exchange(rhs.view, {})),
    allocation(exchange(rhs.allocation, {})),
    info(exchange(rhs.info, {}))
{
}

DeviceImage& DeviceImage::operator=(DeviceImage&& rhs)
{
    if (this!=&rhs)
    {
        swap(allocator, rhs.allocator);
        swap(image, rhs.image);
        swap(view, rhs.view);
        swap(allocation, rhs.allocation);
        swap(info, rhs.info);
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

void BufferManager::upload(const vk::Image& image, const vk::BufferImageCopy& region) const
{
    auto commands=device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    auto& copy=commands.front();
    copy.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    auto barrier = vk::ImageMemoryBarrier
    {
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .image = image,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };
    copy.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {},{},{},
        barrier
    );

    copy.copyBufferToImage(stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    copy.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},{},{},
        barrier
    );

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


