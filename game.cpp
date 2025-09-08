//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#include "game.h"
#include "vulkan.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>

#include <SDL3/SDL_scancode.h>

//! @brief constructor
Game::Game(const filesystem::path& levels, glm::vec2 fieldSize) :
    state(Active),
    keys(),
    fieldSize(fieldSize),
    sprites(3, 1024, 16),
    trail(ceil(TrailEmitsPerSecond*TrailDuration)+1, "textures/circle.png"),
    brickParts(128,  "textures/fragment.png"),
    nextTrailEmit(0.0f)
{
    for (auto const& dir_entry : std::filesystem::directory_iterator{levels})
    {
        if (dir_entry.is_regular_file()) levelList.push_back(dir_entry.path());
    }
    if (levelList.empty()) throw runtime_error("Failed to find any levels");
    ranges::sort(levelList);
    curLevel=levelList.end();

    auto bg=sprites.getOrCreateTexture("background", "textures/background.jpg");
    background=sprites.createSprite(BackgroundLayer, fieldSize*0.5f, bg, fieldSize);
    player=sprites.createSprite(
        GameLayer,
        {}, // position will be set up when level is initialied
        sprites.getOrCreateTexture("paddle","textures/paddle.png"),
        InitialPlayerSize
    );

    ball.radius = InitialBallSize;
    ball.sprite = sprites.createSprite(
        GameLayer,
        {}, // position is reset when level starts
        sprites.getOrCreateTexture("ball", "textures/awesomeface.png"),
        { ball.radius*2.2f, ball.radius*2.2f }
    );

    nextLevel();

    brick[0]=audioManager.loadWav("sounds/brick0.wav"),
    brick[1]=audioManager.loadWav("sounds/brick1.wav"),
    brick[2]=audioManager.loadWav("sounds/brick2.wav"),
    go=audioManager.loadWav("sounds/go.wav");
    lost=audioManager.loadWav("sounds/lost.wav");
    paddle[0]=audioManager.loadWav("sounds/paddle0.wav");
    paddle[1]=audioManager.loadWav("sounds/paddle1.wav");
    solid=audioManager.loadWav("sounds/solid.wav");
    wall[0]=audioManager.loadWav("sounds/wall0.wav");
    wall[1]=audioManager.loadWav("sounds/wall1.wav");
    wall[2]=audioManager.loadWav("sounds/wall2.wav");
}

Game::~Game()
{}

void Game::updateScreenSize(const vk::Extent2D& extent)
{
    glm::vec2 screen={extent.width, extent.height};

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

    auto ortho=glm::orthoRH_ZO(
        -offset.x, viewport.x-offset.x,
        -offset.y, viewport.y-offset.y,
        0.0f, 1.0f);
    sprites.setLayerTransform(BackgroundLayer, ortho);
    sprites.setLayerTransform(GameLayer, ortho);
    sprites.setLayerTransform(ForegroundLayer, ortho);
    trail.setTransformation(ortho);
    brickParts.setTransformation(ortho);
}

void Game::update(float dt, PostProcess& post)
{
    float decay=powf(1.0f-TrailDecayPerSecond,dt);

    trail.update(dt, [dt,decay](auto& p) {
        p.move(p.velocity*dt);
        p.rotate(p.angularVelocity*dt);
        p.velocity*=decay;
        p.angularVelocity*=decay;
        p.color.a*=decay;
    });

    brickParts.update(dt, [dt,decay](auto& p) {
        p.move(p.velocity*dt);
        p.rotate(p.angularVelocity*dt);
        p.velocity.y+=dt*1000.0f;
        p.color.a*=decay;
    });


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

        nextTrailEmit+=TrailEmitsPerSecond*dt;
        while (nextTrailEmit>1.0f)
        {
            auto ofs=glm::linearRand(-TrailPosVar, TrailPosVar);
            trail.spawnParticleP(
                TrailDuration,
                TrailColor,
                bp+ofs,
                glm::linearRand(TrailSizeMin, TrailSizeMax),
                glm::linearRand(0.0f, float(M_PI)*0.5f),
                ofs*20.0f,
                glm::linearRand(-float(M_PI),float(M_PI))*3.0f
            );
            nextTrailEmit-=1.0f;
        }

        bp += ball.velocity * dt;

        // bounce off of walls
        if (bp.x <= ball.radius)
        {
            wall[rand()%3]->play();
            reflectBall(true, ball.radius);
        }
        else if (bp.x >= fieldSize.x-ball.radius)
        {
            wall[rand()%3]->play();
            reflectBall(true, fieldSize.x-ball.radius);
        }

        if (bp.y <= ball.radius)
        {
            wall[rand()%3]->play();
            reflectBall(false, ball.radius);
        }
        else if (bp.y >= fieldSize.y)
        {
            lost->play();
            resetPlayer();
            return;
        }

        // check collision with level and reflect accordingly
        auto [block, closest, isSolid]=level->getBallCollision(bp, ball.radius);
        if (block)
        {
            if (isSolid)
            {
                solid->play();
                post.shake(0.05);
            }
            else
            {
                brick[rand()%3]->play();
                explodeBrick(block->color, block->pos, block->size, closest, glm::length(ball.velocity));
            }

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
            paddle[rand()%2]->play();
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
        if (keys[SDL_SCANCODE_L])
        {
            nextLevel();
            keys[SDL_SCANCODE_L]=false;
        }

        float ds = PlayerVelocity * dt;
        // move playerboard
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        {
            player->pos.x = max(player->pos.x-ds, player->size.x*0.5f);
        }
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        {
            player->pos.x = min(player->pos.x+ds, fieldSize.x-player->size.x*0.5f);
        }

        if (keys[SDL_SCANCODE_SPACE] && ball.stuck)
        {
            ball.stuck=false;
            keys[SDL_SCANCODE_SPACE]=false;
            go->play();
        }
    }
}

void Game::draw(const vk::CommandBuffer& commandBuffer) const
{
    sprites.drawLayer(BackgroundLayer, commandBuffer);
    trail.draw(commandBuffer);
    sprites.drawLayer(GameLayer, commandBuffer);
    brickParts.draw(commandBuffer);
    sprites.drawLayer(ForegroundLayer, commandBuffer);
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

    level=make_unique<Level>(*curLevel, glm::vec2{ fieldSize.x, fieldSize.y/2 }, sprites, GameLayer);
    resetPlayer();
}

void Game::resetPlayer()
{
    player->pos={fieldSize.x*0.5f, fieldSize.y-player->size.y};
    ball.stuck=true;
    ball.velocity=InitialBallVelocity;
}

void Game::explodeBrick(
    const glm::vec4& color,
    const glm::vec2& brickPos,
    const glm::vec2& brickSize,
    const glm::vec2& hitPoint,
    float velocity
)
{
    for (float y=-0.25f; y<0.5f; y+=0.5f)
    {
        for (float x=-0.375f; x<0.5f; x+=0.25f)
        {
            auto center=brickPos+glm::vec2{x,y}*brickSize;
            auto dir=glm::normalize(center-hitPoint);
            brickParts.spawnParticleP(
                1.0f,
                color,
                center,
                glm::vec2{brickSize.x*0.25f, brickSize.y*0.5f},
                glm::linearRand(0.0f, float(M_PI)*2.0f),
                dir*velocity,
                glm::linearRand(-float(M_PI),float(M_PI))*5.0f
            );

        }
    }
}
