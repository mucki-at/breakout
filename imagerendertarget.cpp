//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "imagerendertarget.h"
#include "vulkan.h"

void ImageRenderTarget::reset(const ImageDescription& description, size_t imageCount)
{
    createImages(
        description,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::SampleCountFlagBits::e4,
        imageCount
    );
}

