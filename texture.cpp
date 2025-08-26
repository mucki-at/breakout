//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "texture.h"
#include "stb_image.h"

[[nodiscard]] DeviceImage createImageFromFile(
    const string& filename,
    const BufferManager& bufferManager
)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    auto desc = ImageDescription
    {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .byteSize = static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4,
        .format = vk::Format::eR8G8B8A8Srgb
    };

    auto image = bufferManager.createImage(desc, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
    memcpy(bufferManager.getStage(0, desc.byteSize), pixels, desc.byteSize);
    bufferManager.upload(image, vk::BufferImageCopy{
            .imageExtent = { desc.width, desc.height, 1 },
            .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }
        });
    stbi_image_free(pixels);

    return image;
}