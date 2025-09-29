//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "glm/glm.hpp"
#include "buffermanager.h"

#include <ft2build.h>
#include FT_FREETYPE_H  

class Font
{
public:
    Font(const filesystem::path& filename);
    ~Font();

    void resize(const glm::mat4& transformation, const vk::Extent2D& screenSize, float emSizeInDisplayUnits);
    void renderText(const vk::CommandBuffer& buffer, const glm::vec2& baselinePos, const std::string& ascii) const;

private:
    struct CharacterPushData
    {
        glm::vec2 position=glm::vec2(0.0f, 0.0f);
    };

    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;

    DeviceBuffer constants, vertices;
    DeviceImage atlas;
    array<float,256> glyphAdvances;
    vk::raii::Sampler sampler;

    FT_Library ft;
    FT_Face face;
};
