#pragma once

#include "GameState.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "Level.h"

#include <memory>

/*
 * GameLoop
 * --------
 * Controla el flujo principal del juego.
 *
 * Usa GameStatus para decidir qué renderizar:
 *  MENU, RUNNING, PAUSED, etc.
 */

class GameLoop {
private:
    bool running;

    GameState gs;

    InputHandler input;
    Renderer renderer;

    std::unique_ptr<Level> currentLevel_;

public:
    GameLoop();
    void run();
};