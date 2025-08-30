//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"
#include "level.h"

//! @brief Game holds all game-related state and functionality.
//! Combines all game-related data into a single class for
//! easy access to each of the components and manageability.
class Game
{
public:
    static constexpr size_t KeyCount = 1024;
    static constexpr glm::vec2 InitialBallVelocity = { 100.0f, -350.0f };
    static constexpr float InitialBallSize = 12.5f;
    static constexpr glm::vec2 InitialPlayerSize = { 100.0f, 25.0f };
    static constexpr float PlayerVelocity = 300.0f;

public:
    enum State
    {
        Active,
        Menu,
        Win
    };

    struct Ball
    {
        SpriteManager::Sprite sprite;
        bool stuck;
        float radius;
        glm::vec2 velocity;
    };

public:
    // constructor/destructor
    Game(const filesystem::path& levels, glm::vec2 fieldSize);
    ~Game();

    // game loop
    void updateScreenSize();
    void processInput(float dt);
    void update(float dt);
    void draw(const vk::CommandBuffer& commandBuffer) const;

    inline void setKey(size_t key, bool pressed)
    {
        if (key < KeyCount)
            keys[key] = pressed;
    }

private:
    // game state
    State  state;
    bool   keys[KeyCount];
    glm::vec2 fieldSize;

    // draws all our sprites
    SpriteManager sprites;
    SpriteManager::Sprite background;
    SpriteManager::Sprite player;
    Ball ball;
    void reflectBall(bool horizontal, float limit);

    // level specific data
    vector<filesystem::path> levelList;
    vector<filesystem::path>::const_iterator curLevel;
    unique_ptr<Level> level;
    void resetPlayer();
    void nextLevel();
};
