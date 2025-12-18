#pragma once

class Game
{
public:
    Game() = default;
    ~Game() = default;

    void init();
    void shutdown();

    void update();
private:
};