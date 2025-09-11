//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "rendertarget.h"

class ImageRenderTarget : public MultisampleRenderTarget
{
public:
    using MultisampleRenderTarget::RenderTarget;
    
    void reset(const ImageDescription& description, size_t imageCount);
    using MultisampleRenderTarget::cycle;

    inline DeviceImage& getCurrent() const { return *current; }
};