//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"

class Level
{
public:
    Level(const filesystem::path& file, glm::vec2 topLeft, glm::vec2 blockSize, SpriteManager& sprites, size_t layer);

    bool isComplete();

    tuple<SpriteManager::Sprite,glm::vec2,size_t,size_t> getBallCollision(const glm::vec2& pos, float radius);
    
private:
    struct Brick
    {
        SpriteManager::Sprite sprite;
        size_t score;
        size_t hp;
    };

    SpriteManager::Texture block, solid;
    vector<Brick> bricks;
};