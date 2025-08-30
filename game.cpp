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
    playerSize = {100.0f, 25.0f};
    playerVelocity = 500.0f;

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
        {fieldSize.x*0.5f, fieldSize.y-playerSize.y},
        sprites.getOrCreateTexture("paddle","textures/out.png"),
        playerSize
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

        float movement = playerVelocity * dt;
        // move playerboard
        if (keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_A])
        {
            player->pos.x = max(player->pos.x-movement, playerSize.x*0.5f);
        }
        if (keys[GLFW_KEY_RIGHT] || keys[GLFW_KEY_D])
        {
            player->pos.x = min(player->pos.x+movement, fieldSize.x-playerSize.x*0.5f);
        }
    }
}

void Game::draw(const vk::CommandBuffer& commandBuffer) const
{
    sprites.drawSprites(commandBuffer);
}

void Game::nextLevel()
{
    if (curLevel==levelList.end()) curLevel=levelList.begin();
    else ++curLevel;
    if (curLevel==levelList.end()) curLevel=levelList.begin();

    level=make_unique<Level>(*curLevel, glm::vec2{ fieldSize.x, fieldSize.y/2 }, sprites);
    player->pos={fieldSize.x*0.5f, fieldSize.y-playerSize.y};
}