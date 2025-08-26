//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "swapchainmanager.h"
#include "dynamicresource.h"

template<typename Vertex_, typename Uniforms_, size_t MaxFramesInFlight_ = 2>
class Pipeline
{
public:
    static inline constexpr auto MaxFramesInFlight=MaxFramesInFlight_;
    using Vertex=Vertex_;
    using Uniforms=Uniforms_;

    struct PerFrameData
    {
        Uniforms* uniforms;
        vk::DescriptorSet descriptors;
    };

public:
    Pipeline(
        const vk::raii::Device& device,
        const SwapChainManager& swapper,
        const BufferManager& bufferManager,
        const DeviceImage& texture,
        const vk::raii::Sampler& sampler
    ) :
        pipeline(nullptr),
        layout(nullptr),
        descriptorPool(nullptr),    
        descriptorSets(nullptr),
        descriptorSetLayout(nullptr),
        uniformBuffers(),
        perFrame()
    {
        auto shaderModule=loadShaderModule(device, "shaders/slang.spv");
        vk::PipelineShaderStageCreateInfo shaderStages[]=
        {
            vk::PipelineShaderStageCreateInfo
            {
                .stage = vk::ShaderStageFlagBits::eVertex,
                .module = shaderModule,
                .pName = "vertMain"
            },
            vk::PipelineShaderStageCreateInfo
            {
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = shaderModule,
                .pName = "fragMain"
            }
        };

        auto dynamicStates={vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        auto dynamicState = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates);
        
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{}
            .setVertexBindingDescriptions(bindingDescription)
            .setVertexAttributeDescriptions(attributeDescriptions);

        auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo
        {
            .topology = vk::PrimitiveTopology::eTriangleList
        };
        auto viewportState = vk::PipelineViewportStateCreateInfo{ .viewportCount=1, .scissorCount=1 };
        auto rasterizer = vk::PipelineRasterizationStateCreateInfo
        {
            .cullMode = {}, //vk::CullModeFlagBits::eBack,
            .lineWidth = 1.0f
        };
        auto multisampling=vk::PipelineMultisampleStateCreateInfo{};
        auto colorBlendAttachment=vk::PipelineColorBlendAttachmentState
        {
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
        };
        auto colorBlending=vk::PipelineColorBlendStateCreateInfo{}.setAttachments(colorBlendAttachment);
    
        vk::DescriptorSetLayoutBinding bindings[] =
        {
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
            { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
        };

        descriptorSetLayout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo{}.setBindings(bindings) );
        auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
        pipelineLayoutInfo.setSetLayouts(*descriptorSetLayout);
        layout = vk::raii::PipelineLayout( device, pipelineLayoutInfo );

        auto pipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapper.getFormat()
        };
    
        auto pipelineInfo = vk::GraphicsPipelineCreateInfo
        {
            .pNext = &pipelineRenderingCreateInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = layout
        };

        pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);

        // descriptor sets
        vk::DescriptorPoolSize poolSizes[]=
        {
            { .type=vk::DescriptorType::eUniformBuffer, .descriptorCount=MaxFramesInFlight },
            { .type=vk::DescriptorType::eCombinedImageSampler, .descriptorCount=MaxFramesInFlight }
        };
        auto poolInfo=vk::DescriptorPoolCreateInfo
        {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MaxFramesInFlight,
        }.setPoolSizes(poolSizes);
        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);

        vector<vk::DescriptorSetLayout> layouts(2, *descriptorSetLayout);
        auto allocInfo=vk::DescriptorSetAllocateInfo
        {
            .descriptorPool = descriptorPool
        }.setSetLayouts(layouts);
        descriptorSets=vk::raii::DescriptorSets(device, allocInfo);

        // set up per frame data
        auto imageInfo = vk::DescriptorImageInfo
        {
            .sampler = sampler,
            .imageView = texture,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        for (size_t i=0; i<MaxFramesInFlight; ++i)
        {
            uniformBuffers.emplace_back(bufferManager.createBuffer(
                sizeof(Uniforms),
                vk::BufferUsageFlagBits::eUniformBuffer,
                true
            ));
            auto& buf=uniformBuffers.back();

            auto bufferInfo = vk::DescriptorBufferInfo
            {
                .buffer = buf,
                .offset = 0,
                .range = sizeof(Uniforms)
            };

            std::array descriptorWrites{
                vk::WriteDescriptorSet{
                    .dstSet=descriptorSets[i],
                    .descriptorCount=1,
                    .descriptorType=vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo=&bufferInfo
                },
                vk::WriteDescriptorSet{
                    .dstSet=descriptorSets[i],
                    .dstBinding=1,
                    .descriptorCount=1,
                    .descriptorType=vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo=&imageInfo
                }
            };
            device.updateDescriptorSets(descriptorWrites, {});

            perFrame.current().uniforms = static_cast<Uniforms*>(buf.offset(0));
            perFrame.current().descriptors = descriptorSets[i];
            perFrame.cycle();
        }
    }

    inline const vk::raii::Pipeline& getPipeline() const noexcept { return pipeline; }
    inline const vk::raii::PipelineLayout& getLayout() const noexcept { return layout; }

    inline const PerFrameData& getNextFrame() const noexcept { perFrame.cycle(); return perFrame.current(); }
    inline const PerFrameData& getCurrentFrame() const noexcept { return perFrame.current(); }

private:
    vk::raii::Pipeline pipeline;
    vk::raii::PipelineLayout layout;

    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSetLayout descriptorSetLayout;
    vk::raii::DescriptorSets descriptorSets;
    vector<DeviceBuffer> uniformBuffers;

    DynamicResource<PerFrameData, MaxFramesInFlight> perFrame;
};

