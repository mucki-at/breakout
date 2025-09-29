//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"
#include "level.h"
#include "audiomanager.h"
#include "particlesystem.h"
#include "postprocess.h"
#include "font.h"

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

    static constexpr glm::vec2 LogicalSize = { 40.0f, 30.0f };
    static constexpr glm::vec2 BackgroundSize = { 48.0f, 38.0f };
    static constexpr glm::vec2 FieldPosition = { 2.0f, 2.0f };
    static constexpr glm::vec2 FieldSize = { 26.0f, 28.0f };
    static constexpr glm::vec2 BlockSize = { 2.0f, 1.0f };

    static constexpr glm::vec2 InitialBallVelocity = { 10.0f, -10.0f };
    static constexpr float PowerupBallVelocity = 1.2f;
    static constexpr float InitialBallSize = 0.5f;
    static constexpr glm::vec2 InitialPlayerSize = { 4.0f, 1.0f };
    static constexpr glm::vec2 PowerUpPlayerSize = { 6.0f, 1.0f };
    static constexpr float PlayerVelocity = 20.0f;
 
    static constexpr glm::vec2 PowerupSize = { 4.0f, 1.0f }; 
    static constexpr float PowerupFallSpeed = 3.0f;
    static constexpr glm::vec4 NeutralPowerupColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    static constexpr glm::vec4 GoodPowerupColor = { 0.5f, 0.5f, 1.0f, 1.0f };
    static constexpr glm::vec4 BadPowerupColor = { 1.0f, 0.25f, 0.25f, 1.0f };
    static constexpr float PowerupChance=0.1f;
    
    static constexpr float TrailDuration = .5f;
    static constexpr float TrailDecayPerSecond = 0.99f;      
    static constexpr float TrailEmitsPerSecond = 60.0f;
    static constexpr glm::vec4 TrailColor = { 1.0f, 1.0f, 0.2f, 1.0f };
    static constexpr glm::vec2 TrailSizeMin = { 0.2f, 0.2f };
    static constexpr glm::vec2 TrailSizeMax = { 0.5f, 0.5f };
    static constexpr glm::vec2 TrailPosVar = { 0.3f, 0.3f };
    static constexpr float Gravity = 62.0f;

    static constexpr float FontSize = 1.5f;

    static constexpr glm::vec2 ScoreLabelPos = { 31.0f, 4.0f };
    static constexpr glm::vec2 ScorePos =      { 31.0f, 8.0f };

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
        float stickOffset;
        float radius;
        glm::vec2 velocity;
    };

    struct TrailData
    {
        glm::vec2 velocity;
        glm::f32 angularVelocity;
    };

    struct PowerUp
    {
        enum Type
        {
            None,
            Speed,
            Sticky,
            PassThrough,
            Size,
            Confuse,
            Chaos,


            MAX
        };

        Type type;
        float timeLeft;
    };

    struct PowerUpDefinition
    {
        PowerUp::Type type;
        SpriteManager::Texture texture;
        glm::vec4 color;
        float chance;
        float duration;
    };

public:
    // constructor/destructor
    Game(const filesystem::path& levels);
    ~Game();

    // game loop
    void updateScreenSize(const vk::Extent2D& extent);
    void processInput(float dt);
    void update(float dt, PostProcess& post);
    void updateBall(float dt, PostProcess& post);
    void updatePowerups(float dt, PostProcess& post);
    void draw(const vk::CommandBuffer& commandBuffer) const;

    void maybeSpawnPowerups(const SpriteManager::Sprite& brick);
    void forceSpawnPowerup(PowerUp::Type type, const glm::vec2& pos);

    const PowerUpDefinition& getPowerUpFromTexture(SpriteManager::Texture texture);
    const PowerUpDefinition& getPowerUpFromType(PowerUp::Type type);

    inline void setKey(size_t key, bool pressed)
    {
        if (key < KeyCount)
            keys[key] = pressed;
    }

private:
    // game state
    State  state;
    bool   keys[KeyCount];
    glm::vec2 fieldTL;
    glm::vec2 fieldBR;

    // draws all our sprites
    SpriteManager sprites;
    SpriteManager::Sprite background;
    SpriteManager::Sprite player;
    vector<SpriteManager::Sprite> walls;
    Ball ball;
    ParticleSystem<TrailData> trail,brickParts;
    vector<PowerUpDefinition> powerupDefinitions; 
    vector<SpriteManager::Sprite> floatingPowerups;
    PowerUp activePowerup;
    size_t score;

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
    AudioManager::Audio brick,go,lost,paddle,solid,wall;

    Font font;
};
