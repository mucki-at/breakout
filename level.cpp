//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "level.h"
#include <fstream>
#include <sstream>

Level::Level(const filesystem::path& path, glm::vec2 size, SpriteManager& sprites, size_t layer)
{
    block = sprites.getOrCreateTexture("block", "textures/block.png");
    solid = sprites.getOrCreateTexture("solid", "textures/solid.png");

    // load from file
    unsigned int tileCode;
    string line;
    auto file=ifstream(path);
    std::vector<std::vector<unsigned int>> tileData;
    while (getline(file, line)) // read each line from level file
    {
        std::istringstream sstream(line);
        std::vector<unsigned int> row;
        while (sstream >> tileCode) // read each word separated by spaces
            row.push_back(tileCode);
        tileData.push_back(row);
    }
    
    // calculate dimensions
    unsigned int height = tileData.size();
    unsigned int width  = tileData[0].size();
    float unit_width    = size.x / static_cast<float>(width);
    float unit_height   = size.y / static_cast<float>(height);

    glm::vec4 colors[]=
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.8f, 0.8f, 0.7f, 1.0f },
        { 0.2f, 0.6f, 1.0f, 1.0f },
        { 0.0f, 0.7f, 0.0f, 1.0f },
        { 0.8f, 0.8f, 0.4f, 1.0f },
        { 1.0f, 0.5f, 0.0f, 1.0f }
    };
    unsigned int maxColor = 5;

    // initialize level tiles based on tileData		
    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            if (tileData[y][x] == 0) continue;

            auto color=tileData[y][x];
            bool isSolid=color==1;
            bricks.emplace_back(
                sprites.createSprite(
                    layer, 
                    {unit_width * x + unit_width*0.5f, unit_height * y + unit_height*0.5f},
                    isSolid ? solid:block,
                    {unit_width, unit_height},
                    colors[min(color, maxColor)]
                ),
                isSolid
            );
        }
    }  
}

tuple<SpriteManager::Sprite, glm::vec2, bool> Level::getBallCollision(const glm::vec2& pos, float radius)
{
    for (auto&& b : bricks)
    {
        if (b.sprite)
        {
            glm::vec2 halfBlockSize=b.sprite->size*0.5f;
            // find closest point on sprite
            glm::vec2 closest=pos-b.sprite->pos;    // vector to ball relative to brick
            closest = glm::clamp(closest, -halfBlockSize, halfBlockSize);   // clamped to brick size
            closest += b.sprite->pos; // transform into world coordinates
            if (glm::length(closest-pos) < radius) // hit
            {
                auto result=b.sprite;
                if (!b.solid) b.sprite=nullptr; // destroy block
                return tie(result, closest, b.solid);
            }
        }
    }
    return make_tuple(SpriteManager::Sprite{}, glm::vec2{}, false);
}

bool Level::isComplete()
{
    return ranges::all_of(bricks, [](auto&&b) { return b.solid || b.sprite==nullptr; });
}
