#pragma once
#include "GameState.h"
#include "Level.h"

class CollisionSystem {
public:
    static void checkAll(GameState& gs, const Level& level);

private:
    static void checkPitVsPlatforms    (GameState& gs, const Level& level);
    static void checkPitVsEnemies      (GameState& gs);
    static void checkPitOutOfBounds    (GameState& gs);

    static void damagePlayer(GameState& gs, int amount);
    static void damageEnemy (Enemy& e, int amount, GameState& gs);
};