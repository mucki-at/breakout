//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#include "game.h"
#include "vulkan.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <GLFW/glfw3.h>

//! @brief constructor
Game::Game(const filesystem::path& levels, glm::vec2 fieldSize) :
    state(Active),
    keys(),
    fieldSize(fieldSize),
    sprites(1024, 16)
{
    for (auto const& dir_entry : std::filesystem::directory_iterator{levels})
    {
        if (dir_entry.is_regular_file()) levelList.push_back(dir_entry.path());
    }
    if (levelList.empty()) throw runtime_error("Failed to find any levels");
    ranges::sort(levelList);
    curLevel=levelList.end();

    auto bg=sprites.getOrCreateTexture("background", "textures/background.jpg");
    background=sprites.createSprite(fieldSize*0.5f, bg, fieldSize);
    player=sprites.createSprite(
        {}, // position will be set up when level is initialied
        sprites.getOrCreateTexture("paddle","textures/out.png"),
        InitialPlayerSize
    );

    ball.radius = InitialBallSize;
    ball.sprite = sprites.createSprite(
        {}, // position is reset when level starts
        sprites.getOrCreateTexture("ball", "textures/awesomeface.png"),
        { ball.radius*2.2f, ball.radius*2.2f }
    );
    nextLevel();
}

Game::~Game()
{}

void Game::updateScreenSize()
{
    glm::vec2 screen={vulkan.getViewport().width, vulkan.getViewport().height};

    float fieldAspect=fieldSize.x/fieldSize.y;
    float screenAspect=screen.x/screen.y;

    glm::vec2 viewport;
    if (fieldAspect > screenAspect) // field is wider than screen
    {
        viewport.x = fieldSize.x;
        viewport.y = fieldSize.x/screenAspect;
    }
    else
    {
        viewport.x = fieldSize.y*screenAspect;
        viewport.y = fieldSize.y;
    }
    glm::vec2 offset=(viewport-fieldSize)*0.5f;

    sprites.transformation=glm::orthoRH_ZO(
        -offset.x, viewport.x-offset.x,
        -offset.y, viewport.y-offset.y,
        0.0f, 1.0f);
}

void Game::update(float dt)
{
    if (level->isComplete()) nextLevel();

    if (ball.stuck)
    {
        ball.sprite->pos.x=player->pos.x;
        ball.sprite->pos.y=player->pos.y-player->size.y*0.5f-ball.radius;
    }
    else
    {
        // move ball 
        auto& bp=ball.sprite->pos;
        bp += ball.velocity * dt;

        // bounce off of walls
        if (bp.x <= ball.radius) reflectBall(true, ball.radius);
        else if (bp.x >= fieldSize.x-ball.radius) reflectBall(true, fieldSize.x-ball.radius);

        if (bp.y <= ball.radius) reflectBall(false, ball.radius);
        else if (bp.y >= fieldSize.y)
        {
            resetPlayer();
            return;
        }

        // check collision with level and reflect accordingly
        auto [block, closest]=level->getBallCollision(bp, ball.radius);
        if (block)
        {
            glm::vec2 impactDirection = closest-ball.sprite->pos;
            // TODO: handle corners better
            if (fabs(impactDirection.x) > fabs(impactDirection.y))  // reflect horizontally
            {
                if (impactDirection.x > 0) reflectBall(true, closest.x-ball.radius);
                else reflectBall(true, closest.x+ball.radius);
            }
            else
            {
                if (impactDirection.y > 0) reflectBall(false, closest.y-ball.radius);
                else reflectBall(false, closest.y+ball.radius);
            }
        }   
        
        // check paddle collision
        glm::vec2 halfPlayerSize=player->size*0.5f;
        // find closest point on sprite
        glm::vec2 playerHitPos=bp-player->pos;    // vector to ball relative to player
        playerHitPos = glm::clamp(playerHitPos, -halfPlayerSize, halfPlayerSize);   // clamped to player size
        if (glm::length((playerHitPos+player->pos)-bp) < ball.radius) // hit
        {
            reflectBall(false, player->pos.y-halfPlayerSize.y-ball.radius);

            // check where it hit the board, and change velocity based on where it hit the board
            float percentage = playerHitPos.x / halfPlayerSize.x;
            // then move accordingly
            float strength = 2.0f;
            float oldVelocity = glm::length(ball.velocity);
            ball.velocity.x = InitialBallVelocity.x * percentage * strength; 
            ball.velocity = glm::normalize(ball.velocity) * oldVelocity;         
        } 
    }
}

void Game::processInput(float dt)
{
    if (state == Active)
    {
        if (keys[GLFW_KEY_L])
        {
            nextLevel();
            keys[GLFW_KEY_L]=false;
        }

        float ds = PlayerVelocity * dt;
        // move playerboard
        if (keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_A])
        {
            player->pos.x = max(player->pos.x-ds, player->size.x*0.5f);
        }
        if (keys[GLFW_KEY_RIGHT] || keys[GLFW_KEY_D])
        {
            player->pos.x = min(player->pos.x+ds, fieldSize.x-player->size.x*0.5f);
        }

        if (keys[GLFW_KEY_SPACE] && ball.stuck)
        {
            ball.stuck=false;
            keys[GLFW_KEY_SPACE]=false;
        }
    }
}

void Game::draw(const vk::CommandBuffer& commandBuffer) const
{
    sprites.drawSprites(commandBuffer);
}

void Game::reflectBall(bool horizontal, float limit)
{
    auto& bp=ball.sprite->pos;
    if (horizontal)
    {
        ball.velocity.x = -ball.velocity.x;
        bp.x=2.0f*limit-bp.x;
    }
    else
    {
        ball.velocity.y = -ball.velocity.y;
        bp.y=2.0f*limit-bp.y;
    }
}

void Game::nextLevel()
{
    if (curLevel==levelList.end()) curLevel=levelList.begin();
    else ++curLevel;
    if (curLevel==levelList.end()) curLevel=levelList.begin();

    level=make_unique<Level>(*curLevel, glm::vec2{ fieldSize.x, fieldSize.y/2 }, sprites);
    resetPlayer();
}

void Game::resetPlayer()
{
    player->pos={fieldSize.x*0.5f, fieldSize.y-player->size.y};
    ball.stuck=true;
    ball.velocity=InitialBallVelocity;
}
