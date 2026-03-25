#include "game/player.h"

Player::Player(InputManager &inputManager, float aspectRatio)
    : inputManager_(inputManager)
{
    camera_.SetPerspective(glm::radians(60.0f), aspectRatio, 0.01f, 1000.0f);
    camera_.SetPosition(vec3(0.0f, 0.0f, 1.0f));
    camera_.SetRotation(vec3(glm::radians(10.0f), 0.0f, 0.0f));
}

void Player::Update(float deltaTime)
{
}

void Player::Damage(int amount)
{
    // health_ -= amount;
}

void Player::Shoot()
{
    // TODO:
    // 1. cast a ray from camera
    // 2. get entity that was hit
    // 3. get limb of an entity that was hit (optional, maybe for later)
    // 4. apply damage() to entity based on what limb was hit
}