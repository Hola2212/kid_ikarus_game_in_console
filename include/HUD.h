#pragma once
#include "GameState.h"

// ─────────────────────────────────────────────────────────────────────────────
// HUD — Hilo dedicado a actualizar los datos que se mostrarán en pantalla.
//
// No dibuja directamente; prepara un snapshot del estado para que el
// Renderer lo consuma de forma segura (bajo renderMutex).
// ─────────────────────────────────────────────────────────────────────────────
struct HUDData {
    int lives{MAX_LIVES};
    int hp{MAX_HP};
    int hearts{0};
    int phase{1};
    int score{0};
    GameStatus status{GameStatus::RUNNING};
};

class HUD {
public:
    static void threadFunc(GameState* gs);

    // Copia atómica del snapshot más reciente (thread-safe)
    static HUDData getSnapshot();

private:
    static HUDData snapshot_;
    static std::mutex snapshotMutex_;
};
