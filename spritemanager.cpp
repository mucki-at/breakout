//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "spritemanager.h"
#include "pipelinebuilder.h"
#include "vulkan.h"

SpriteManager::SpriteManager(
    size_t maxSprites,
    size_t maxTextures
) :
    pipelineLayout(nullptr),
    pipeline(nullptr),
    descriptorLayout(nullptr),
    descriptorPool(nullptr),
    descriptors(nullptr)
{
    if (maxTextures>256) throw runtime_error("SpriteManager cannot handle more than 256 textures.");

    sprites.reserve(maxSprites);
    textures.reserve(maxTextures);

    DescriptorSetBuilder descBuilder;
    descBuilder.bindings.emplace_back(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    tie(descriptorLayout, descriptorPool, descriptors)=descBuilder.buildLayoutAndSets(vulkan.getDevice(), 256);

    PipelineLayoutBuilder layoutBuilder;
    layoutBuilder.descriptorSets.push_back(descriptorLayout);
    layoutBuilder.pushConstants.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, sizeof(transformation)+sizeof(SpritePushData));
    pipelineLayout=layoutBuilder.build(vulkan.getDevice());

    PipelineBuilder builder;
    auto shaderModule=loadShaderModule(vulkan.getDevice(), "shaders/sprites.spv");        
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eVertex, .module=shaderModule, .pName="vertMain"});
    builder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eFragment, .module=shaderModule, .pName="fragMain"});

    builder.addColorAttachment(
        vulkan.getSwapChain().getFormat(),
        vk::PipelineColorBlendAttachmentState{
            .blendEnable = true,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcColorBlendFactor = vk::BlendFactor::eOne,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
        }
    );
    pipeline = builder.build(vulkan.getDevice(),pipelineLayout);
}

glm::u8 SpriteManager::createTexture(const std::string& filename)
{
    if (textures.size()==256) throw runtime_error("Texture limit exceeded");
    textures.emplace_back(createImageFromFile(filename, vulkan.getBufferManager()), createSampler(vulkan.getPhysicalDevice(), vulkan.getDevice()));

    glm::u8 textureId=textures.size()-1;

    auto imageInfo = vk::DescriptorImageInfo
    {
        .sampler = textures[textureId].sampler,
        .imageView = textures[textureId].image,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet=descriptors[textureId],
            .dstBinding=0,
            .descriptorCount=1,
            .descriptorType=vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo=&imageInfo
        }
    };
    vulkan.getDevice().updateDescriptorSets(descriptorWrites, {});
    return textureId;
}

SpriteManager::Sprite SpriteManager::createSprite(
    glm::vec2 pos,
    glm::u8 texture,
    glm::vec2 size,
    glm::vec4 color
)
{
    if (size.x==0)
    {
        if (texture>=textures.size()) throw runtime_error("texture must be registered to use automatic sprite size");
        auto&& desc=textures[texture].image.getDescription();
        size={ desc.width, desc.height };
    }
    if (sprites.size()<sprites.capacity())
    {
        sprites.emplace_back(pos,size,color,texture);
        return makeSprite(sprites.end()-1);
    }
    for (auto iter=sprites.begin(); iter!=sprites.end(); ++iter)
    {
        if (iter->valid == false)
        {
            iter->pos=pos;
            iter->size=size;
            iter->color=color;
            iter->textureId=texture;
            iter->valid=true;
            return makeSprite(iter);
        }
    }
    throw runtime_error("out of sprites");
}

void SpriteManager::drawSprites(const vk::CommandBuffer& buffer) const
{
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    buffer.pushConstants<glm::mat4>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, transformation);
    for (auto&& s : sprites)
    {
        if (!s.valid) continue;

        buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptors[s.textureId], {});

        buffer.pushConstants<SpritePushData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(transformation), s);
        buffer.draw(4,1,0,0);
    }
}
