//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"
#include "level.h"
#include "audiomanager.h"
#include "particlesystem.h"

//! @brief Game holds all game-related state and functionality.
//! Combines all game-related data into a single class for
//! easy access to each of the components and manageability.
class Game
{
public:
    static constexpr size_t KeyCount = 1024;

    static constexpr size_t BackgroundLayer = 0;
    static constexpr size_t GameLayer = 1;
    static constexpr size_t ForegroundLayer = 2;

    static constexpr glm::vec2 InitialBallVelocity = { 100.0f, -350.0f };
    static constexpr float InitialBallSize = 12.5f;
    static constexpr glm::vec2 InitialPlayerSize = { 100.0f, 25.0f };
    static constexpr float PlayerVelocity = 300.0f;

    static constexpr float TrailDuration = .5f;
    static constexpr float TrailDecayPerSecond = 0.99f;      
    static constexpr float TrailEmitsPerSecond = 60.0f;
    static constexpr glm::vec4 TrailColor = { 1.0f, 1.0f, 0.2f, 1.0f };
    static constexpr glm::vec2 TrailSizeMin = { 5.0f, 5.0f };
    static constexpr glm::vec2 TrailSizeMax = { 15.0f, 15.0f };
    static constexpr glm::vec2 TrailPosVar = { 3.0f, 3.0f };

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

    struct TrailData
    {
        glm::vec2 velocity;
        glm::f32 angularVelocity;
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
    ParticleSystem<TrailData> trail,brickParts;
    float nextTrailEmit;
    
    void reflectBall(bool horizontal, float limit);
    void explodeBrick(
        const glm::vec4& color,
        const glm::vec2& brickPos,
        const glm::vec2& brickSize,
        const glm::vec2& hitPoint,
        float velocity
    );

    // level specific data
    vector<filesystem::path> levelList;
    vector<filesystem::path>::const_iterator curLevel;
    unique_ptr<Level> level;
    void resetPlayer();
    void nextLevel();

    AudioManager audioManager;
    AudioManager::Audio go,dink,solid,lost;
};
