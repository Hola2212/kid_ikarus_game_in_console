#pragma once
#include "GameState.h"
#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
// Physics — Hilo dedicado al movimiento y física de Pit.
// ─────────────────────────────────────────────────────────────────────────────
class Physics {
public:
    // Función de hilo: llama a tick() en bucle mientras gs.running == true
    static void threadFunc(GameState* gs, const Level* level);

private:
    static void tick(GameState& gs, const Level& level);
    static void applyGravity(Player& pit);
    static bool isOnPlatform(const Player& pit, const std::vector<Level::Platform>& plats);
    static void clampToBounds(Player& pit);
};
