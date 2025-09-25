//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "font.h"
#include "vulkan.h"
#include "vkutils.h"
#include "pipelinebuilder.h"

namespace
{
    struct GlyphVertex
    {
        glm::vec2 pos;
        glm::vec2 texcoord;
    };

    struct GlyphImage
    {
        uint32_t x,y,width,height;
    };
}

Font::Font(const filesystem::path& filename) :
    pipelineLayout(nullptr),
    pipeline(nullptr),
    descriptorLayout(nullptr),
    descriptorPool(nullptr),
    descriptors(nullptr),
    constants(vulkan.getBufferManager().createBuffer(
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        false
    )),
    vertices(vulkan.getBufferManager().createBuffer(
        sizeof(GlyphVertex)*4*256,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        false
    )),
    atlas(vulkan.getBufferManager().createImage(
        { .extent = { 1024, 1024}, .format = vk::Format::eR8Unorm},
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    )),
    sampler(createSampler(vulkan.getPhysicalDevice(), vulkan.getDevice())),
    ft(nullptr),
    face(nullptr)

{
    if (FT_Init_FreeType(&ft)) throw runtime_error("ERROR::FREETYPE: Could not init FreeType Library");
    if (FT_New_Face(ft, filename.c_str(), 0, &face))
    {
        FT_Done_FreeType(ft);
        ft=nullptr;
        throw runtime_error("ERROR::FREETYPE: Failed to load font '"+filename.string()+"'");  
    }

    DescriptorSetBuilder descBuilder;
    descBuilder.bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    descBuilder.bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    tie(descriptorLayout, descriptorPool, descriptors)=descBuilder.buildLayoutAndSets(vulkan.getDevice(), 1);

    PipelineLayoutBuilder layoutBuilder;
    layoutBuilder.descriptorSets.push_back(descriptorLayout);
    layoutBuilder.pushConstants.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, sizeof(CharacterPushData));
    pipelineLayout=layoutBuilder.build(vulkan.getDevice());

    PipelineBuilder builder;
    builder.vertexInputBindings.emplace_back(0, sizeof(GlyphVertex), vk::VertexInputRate::eVertex);
    builder.vertexInputAttributes.emplace_back(0, 0, vk::Format::eR32G32Sfloat, offsetof(GlyphVertex, pos));
    builder.vertexInputAttributes.emplace_back(1, 0, vk::Format::eR32G32Sfloat, offsetof(GlyphVertex, texcoord));

    auto shaderModule=loadShaderModule(vulkan.getDevice(), "shaders/text.spv");        
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eVertex, .module=shaderModule, .pName="vertMain"});
    builder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eFragment, .module=shaderModule, .pName="fragMain"});

    builder.multisample.rasterizationSamples = vk::SampleCountFlagBits::e4;

    builder.addColorAttachment(
        vulkan.getSwapChainFormat().format,
        vk::PipelineColorBlendAttachmentState{
            .blendEnable = true,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
        }
    );
    pipeline = builder.build(vulkan.getDevice(),pipelineLayout);

    auto constantInfo = vk::DescriptorBufferInfo
    {
        .buffer = constants,
        .offset = 0,
        .range = sizeof(glm::mat4)
    };

    auto imageInfo = vk::DescriptorImageInfo
    {
        .sampler = sampler,
        .imageView = atlas,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet=descriptors[0],
            .dstBinding=0,
            .descriptorCount=1,
            .descriptorType=vk::DescriptorType::eUniformBuffer,
            .pBufferInfo=&constantInfo,
        },
        vk::WriteDescriptorSet{
            .dstSet=descriptors[0],
            .dstBinding=1,
            .descriptorCount=1,
            .descriptorType=vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo=&imageInfo
        }
    };
    vulkan.getDevice().updateDescriptorSets(descriptorWrites, {});

}

Font::~Font()
{
    if (face!=nullptr)
    {
        FT_Done_Face(face);
        face=nullptr;
    }
    if (ft!=nullptr)
    {
        FT_Done_FreeType(ft);
        ft=nullptr;
    }
}

void Font::resize(const glm::mat4& transformation, const glm::vec2& logicalScreenSize, float emSizeInLogicalUnits)
{
    glm::vec2 unitVectorX = { transformation[0][0], transformation[0][1] };
    glm::vec2 unitVectorY = { transformation[1][0], transformation[1][1] };

    float pixelDensityX = logicalScreenSize.x*glm::length(unitVectorX);
    float pixelDensityY = logicalScreenSize.y*glm::length(unitVectorY);

    if (FT_Set_Pixel_Sizes(face, emSizeInLogicalUnits*pixelDensityX, emSizeInLogicalUnits*pixelDensityX))
        throw runtime_error("ERROR::Freetype: Failed set font size");

    // pack all glyphs into a single texture
    size_t padding=1; // padding pixels for each character
    auto texSize=atlas.getDescription().extent;
    vector<GlyphImage> glyphs;
    uint32_t posX=0, posY=0, lineHeight=0;

    // step 1: run over all glyphs and calculate position in the texture. resize if necessary.
    for (FT_ULong c = 0; c < 256; ++c)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, 0)) throw runtime_error("ERROR::FREETYTPE: Failed to load Glyph");

        uint32_t widthInImage=face->glyph->bitmap.width+2*padding;
        if (posX+widthInImage > texSize.width)
        {
            posX=0;
            posY+=lineHeight;
            if (posX+widthInImage > texSize.width) throw runtime_error("ERROR:FREETYPE: Glyph can never fit in texture!");
        }

        uint32_t heightInImage=face->glyph->bitmap.rows+2*padding;
        glyphs.emplace_back(posX+padding, posY+padding, face->glyph->bitmap.width, face->glyph->bitmap.rows);
        lineHeight=max(lineHeight, heightInImage);

        posX+=padding*2+face->glyph->bitmap.width;

        glyphAdvances[c] = ceil(float(face->glyph->advance.x)/(64.0f*pixelDensityX));
    }

    // Step 2: adapt texture if necessary
    if (posY+lineHeight > texSize.height)
    {
        atlas = vulkan.getBufferManager().createImage(
            { .extent = { texSize.width, posY+lineHeight }, .format = vk::Format::eR8Unorm},
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        );
        texSize=atlas.getDescription().extent;

        auto imageInfo = vk::DescriptorImageInfo
        {
            .sampler = sampler,
            .imageView = atlas,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        std::array descriptorWrites{
            vk::WriteDescriptorSet{
                .dstSet=descriptors[0],
                .dstBinding=1,
                .descriptorCount=1,
                .descriptorType=vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo=&imageInfo
            }
        };
        vulkan.getDevice().updateDescriptorSets(descriptorWrites, {});
    }

    // Step 3: fill vertex buffer
    vulkan.getBufferManager().resizeStage(4*256*sizeof(GlyphVertex));
    for (FT_ULong c = 0; c < 256; ++c)
    {
        float left = float(face->glyph->bitmap_left)/pixelDensityX;
        float top  = float(-face->glyph->bitmap_top)/pixelDensityY;
        float right = float(face->glyph->bitmap_left + face->glyph->bitmap.width)/pixelDensityX;
        float bottom = float(face->glyph->bitmap.rows - face->glyph->bitmap_top)/pixelDensityY;

        float texLeft = float(glyphs[c].x) / float(texSize.width);
        float texTop = float(glyphs[c].y) / float(texSize.width);
        float texRight = float(glyphs[c].x+glyphs[c].width) / float(texSize.width);
        float texBottom = float(glyphs[c].y+glyphs[c].height) / float(texSize.width);
 
        GlyphVertex quad[4]={
            { glm::vec2{left, top}, glm::vec2{texLeft, texTop} },
            { glm::vec2{right, top}, glm::vec2{texRight, texTop} },
            { glm::vec2{left, bottom}, glm::vec2{texLeft, texBottom} },
            { glm::vec2{right, bottom}, glm::vec2{texRight, texBottom} }
        };
        memcpy(vulkan.getBufferManager().getStage(c*sizeof(quad), sizeof(quad)), quad, sizeof(quad));
    }
    vulkan.getBufferManager().upload(vertices, vk::BufferCopy{.size=4*256*sizeof(GlyphVertex)});

    // Step 4: render glyphs into texture and setup texture coordinates
    byte* stage=static_cast<byte*>(vulkan.getBufferManager().getStage(0, texSize.width * texSize.height));
    for (FT_ULong c = 0; c < 256; ++c)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) throw runtime_error("ERROR::FREETYTPE: Failed to load Glyph");

        for (size_t row=0; row<face->glyph->bitmap.rows; ++row)
        {
            memcpy(stage+texSize.width*(glyphs[c].y+row)+glyphs[c].x,face->glyph->bitmap.buffer+row*face->glyph->bitmap.pitch, glyphs[c].width);
        }
    }
    vulkan.getBufferManager().upload(
        atlas,
        vk::BufferImageCopy{
            .imageExtent = { texSize.width, texSize.height, 1 },
            .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }
        });

    // Step 5: update our constants
    memcpy(vulkan.getBufferManager().getStage(0, sizeof(glm::mat4)), &transformation, sizeof(transformation));
    vulkan.getBufferManager().upload(constants, vk::BufferCopy{.size=sizeof(glm::mat4)});

}

void Font::renderText(const vk::CommandBuffer& buffer, const glm::vec2& baselinePos, const std::string& ascii) const
{
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptors[0], {});
    buffer.bindVertexBuffers(0, {vertices}, {0});
    
    CharacterPushData data;
    data.position=baselinePos;

    for (auto&& c : ascii)
    {
        buffer.pushConstants<CharacterPushData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, data);
        buffer.draw(4,1,c*4,0);
        data.position.x += glyphAdvances[c];
    }

}

