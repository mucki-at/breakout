//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "particlesystem.h"
#include "pipelinebuilder.h"
#include "vulkan.h"
#include "vkutils.h"
#include "glm/gtc/matrix_transform.hpp"

namespace detail
{

ParticleSystemBase::ParticleSystemBase(
    const filesystem::path& texture
) :
    pipelineLayout(nullptr),
    pipeline(nullptr),
    descriptorLayout(nullptr),
    descriptorPool(nullptr),
    descriptors(nullptr),
    image(createImageFromFile(texture, vulkan.getBufferManager())),
    sampler(createSampler(vulkan.getPhysicalDevice(), vulkan.getDevice())),
    transformation(1.0f)
{
    DescriptorSetBuilder descBuilder;
    descBuilder.bindings.emplace_back(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    tie(descriptorLayout, descriptorPool, descriptors)=descBuilder.buildLayoutAndSets(vulkan.getDevice(), 1);

    PipelineLayoutBuilder layoutBuilder;
    layoutBuilder.descriptorSets.push_back(descriptorLayout);
    layoutBuilder.pushConstants.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)+sizeof(ParticlePushData));
    pipelineLayout=layoutBuilder.build(vulkan.getDevice());

    PipelineBuilder builder;
    auto shaderModule=loadShaderModule(vulkan.getDevice(), "shaders/particles.spv");        
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eVertex, .module=shaderModule, .pName="vertMain"});
    builder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eFragment, .module=shaderModule, .pName="fragMain"});

    builder.addColorAttachment(
        vulkan.getSwapChainFormat().format,
        vk::PipelineColorBlendAttachmentState{
            .blendEnable = true,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOne,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
        }
    );
    pipeline = builder.build(vulkan.getDevice(),pipelineLayout);

    auto imageInfo = vk::DescriptorImageInfo
    {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet=descriptors[0],
            .dstBinding=0,
            .descriptorCount=1,
            .descriptorType=vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo=&imageInfo
        }
    };
    vulkan.getDevice().updateDescriptorSets(descriptorWrites, {});
}

} // end namespace detail