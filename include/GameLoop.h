#pragma once

#include "GameState.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "Level.h"
#include "ScoreManager.h"

#include "EnemyManager.h"
#include "Physics.h"
#include "ProjectileManager.h"
#include "HUD.h"
#include "CpuPlayer.h"

#include <memory>
#include <thread>

/*
 * GameLoop
 * --------
 * Controla el flujo principal del juego.
 */

class GameLoop {

private:

    // Control principal del loop
    bool running;

    // Estado global
    GameState gs;

    // Sistemas
    InputHandler input;
    Renderer renderer;

    // Nivel actual
    std::unique_ptr<Level> currentLevel_;

    // ============================
    // THREADS DEL JUEGO
    // ============================

    std::thread enemyThread_;
    std::thread physicsThread_;
    std::thread projPitThread_;
    std::thread projEnemyThread_;
    std::thread hudThread_;

    // ============================
    // HELPERS
    // ============================

    void resetGame();

public:

    GameLoop();

    ~GameLoop();

    void run();
};