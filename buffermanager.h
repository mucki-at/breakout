//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once
#include "common.h"
#include "vma.h"

class DeviceBuffer
{
public:
    ~DeviceBuffer();

    DeviceBuffer(const DeviceBuffer& rhs) = delete;
    DeviceBuffer& operator=(const DeviceBuffer& rhs) = delete;

    DeviceBuffer(DeviceBuffer&& rhs);
    DeviceBuffer& operator=(DeviceBuffer&& rhs);

    inline operator vk::Buffer() const noexcept { return buffer; }
    [[nodiscard]] inline void* offset(size_t ofs) const { return static_cast<byte*>(info.pMappedData)+ofs; }

private:
    vma::Allocator allocator;
    vk::Buffer buffer;
    vma::Allocation allocation;
    vma::AllocationInfo info;

    DeviceBuffer(
        vma::Allocator allocator,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        const vma::AllocationCreateInfo& allocInfo
    );

    friend class BufferManager;
};

struct ImageDescription
{
    uint32_t width;
    uint32_t height;
    vk::DeviceSize byteSize;
    vk::Format format;
};

class DeviceImage
{
public:
    ~DeviceImage();

    DeviceImage(const DeviceImage& rhs) = delete;
    DeviceImage& operator=(const DeviceImage& rhs) = delete;

    DeviceImage(DeviceImage&& rhs);
    DeviceImage& operator=(DeviceImage&& rhs);

    inline operator vk::Image() const noexcept { return image; }
    inline operator vk::ImageView() const noexcept { return view; }
    [[nodiscard]] inline void* offset(size_t ofs) const { return static_cast<byte*>(info.pMappedData)+ofs; }

private:
    vma::Allocator allocator;
    vk::Image image;
    vk::ImageView view;
    vma::Allocation allocation;
    vma::AllocationInfo info;

    DeviceImage(
        vma::Allocator allocator,
        vk::Device device,
        const ImageDescription& description,
        vk::ImageUsageFlags usage,
        const vma::AllocationCreateInfo& allocInfo
    );

    friend class BufferManager;
};

class BufferManager
{
public:
    BufferManager(
        const vma::Allocator& allocator,
        const vk::raii::Device& device,
        const vk::raii::Queue& transferQueue,
        uint32_t transferQueueFamily,
        vk::DeviceSize initialStagingSize=1024
    );

    inline DeviceBuffer createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        const vma::AllocationCreateInfo& allocInfo
    ) const
    {
        return DeviceBuffer(allocator, size, usage, allocInfo);
    }

    inline DeviceBuffer createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        bool cpuAccess
    ) const
    {
        return createBuffer(size, usage, vma::AllocationCreateInfo{
            .usage = vma::MemoryUsage::eAuto,
            .flags = (cpuAccess ? vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped : vma::AllocationCreateFlags())
            });
    }

    inline DeviceImage createImage(
        const ImageDescription& description,
        vk::ImageUsageFlags usage,
        const vma::AllocationCreateInfo& allocInfo = { .usage=vma::MemoryUsage::eAuto }
    ) const
    {
        return DeviceImage(allocator, device, description, usage, allocInfo);
    }

    void resizeStage(vk::DeviceSize minSize) const;
    inline void* getStage(size_t offset, size_t bytes) const
    {
        if (offset+bytes > stagingBuffer.info.size) resizeStage(offset+bytes);
        return stagingBuffer.offset(offset);
    }
    
    void upload(const vk::Buffer& buffer, const vk::BufferCopy& range) const;
    void upload(const vk::Image& image, const vk::BufferImageCopy& region) const;

private:
    vma::Allocator allocator;
    const vk::raii::Device& device;
    const vk::raii::Queue& transferQueue;
    vk::raii::CommandPool commandPool;

    mutable DeviceBuffer stagingBuffer;
    vk::raii::Fence fence;
};