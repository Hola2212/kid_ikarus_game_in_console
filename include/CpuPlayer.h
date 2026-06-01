#pragma once
#include "GameState.h"
#include "Level.h"

class CpuPlayer {
public:
    static void tick(GameState& gs, const Level& level);
};
