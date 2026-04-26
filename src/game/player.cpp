#include "game/player.h"

Player::Player(Input &inputManager, float aspectRatio)
    : m_inputManager(inputManager)
{
    m_camera.SetPerspective(glm::radians(60.0f), aspectRatio, 0.01f, 1000.0f);
    m_camera.SetPosition(vec3(0.0f, 0.0f, 1.0f));
    m_camera.SetRotation(vec3(glm::radians(10.0f), 0.0f, 0.0f));
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