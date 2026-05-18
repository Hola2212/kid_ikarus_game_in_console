#pragma once

#include "GameState.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "Level.h"
#include "ScoreManager.h"

#include <memory>

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

public:

    GameLoop();

    void run();
};