//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "texture.h"
#include <glm/glm.hpp>

class SpriteManager
{
public:
    static constexpr inline glm::vec2 Automatic = glm::vec2{};

private:
    struct SpritePushData
    {
        glm::vec2 pos;
        glm::vec2 size;
        glm::vec4 color;
    };

public:
    using Texture = glm::u8;
    struct SpriteData : public SpritePushData
    {
        Texture texture = 0;

        SpriteData(
            const glm::vec2& pos,
            const glm::vec2& size,
            const glm::vec4& color,
            Texture texture
        ) :
            SpritePushData(pos, size, color),
            texture(texture)
        {}

    };

private:
    struct SpriteEntry : public SpriteData
    {
    public:
        SpriteEntry(
            const glm::vec2& pos,
            const glm::vec2& size,
            const glm::vec4& color,
            Texture texture
        ) :
            SpriteData(pos, size, color, texture),
            valid(true)
        {}

    private:
        bool valid=false;
        friend class SpriteManager;
    };

    struct TextureEntry
    {
        Texture id;
        DeviceImage image;
        vk::raii::Sampler sampler;
    };

public:
    using Sprite = shared_ptr<SpriteEntry>;

private:
    using Container = vector<SpriteEntry>;
    inline Sprite makeSprite(Container::iterator iter)
    {
        return Sprite(&*iter, [](SpriteEntry* e){e->valid=false;});
    };

public:
    SpriteManager(size_t maxSprites, size_t maxTextures=256);

//    Texture recreateTexture(const string& name, const filesystem::path& filename);
    Texture getOrCreateTexture(const string& name, const filesystem::path& filename);
//    void releaseTexture(const string& name);

    Sprite createSprite(
        glm::vec2 pos,
        Texture texture,
        glm::vec2 size = Automatic,
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }
    );

    void drawSprites(const vk::CommandBuffer& buffer) const;

    glm::mat4 transformation;

private:
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;
    
    Container sprites;
    map<string, TextureEntry> textures;
    vector<Texture> freeTextureIds;

    Texture createTextureEntry(const string& name, const filesystem::path& filename);
};