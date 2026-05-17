#include "GameLoop.h"
#include "ScoreManager.h" 

#include <chrono>
#include <thread>

/*
 * Constructor
 */
GameLoop::GameLoop() {
    running = true;

    // Crear nivel con fase actual
    currentLevel_ = std::make_unique<Level>(1);

    // Inicialización del jugador
    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);

        gs.pit.pos.x = SCREEN_WIDTH / 2;
        gs.pit.pos.y = SCREEN_HEIGHT - 2;

        gs.pit.velY = 0;

        gs.pit.hp = MAX_HP;
        gs.pit.lives = MAX_LIVES;
        gs.pit.onGround = true;
    }

    // Estado inicial = menú
    gs.running.store(true);
    gs.status.store(GameStatus::MENU);
}

/*
 * Loop principal
 */
void GameLoop::run() {

    while (running && gs.running.load()) {

        // =====================
        // INPUT
        // =====================
        input.processInput(gs);

        // =====================
        // UPDATE (solo si está jugando)
        // =====================
        if (gs.status.load() == GameStatus::RUNNING) {

            std::lock_guard<std::mutex> lock(gs.pitMutex);

            // Gravedad
            if (!gs.pit.onGround) {
                gs.pit.velY += 1;
                gs.pit.pos.y += gs.pit.velY;
            }

            // Suelo
            if (gs.pit.pos.y >= SCREEN_HEIGHT - 2) {
                gs.pit.pos.y = SCREEN_HEIGHT - 2;
                gs.pit.velY = 0;
                gs.pit.onGround = true;
            }

            // Límite superior
            if (gs.pit.pos.y < 0) {
                gs.pit.pos.y = 0;
                gs.pit.velY = 0;
            }
        }

        // =====================
        // RENDER SEGÚN ESTADO
        // =====================
        {
            std::lock_guard<std::mutex> lock(gs.renderMutex);

            switch (gs.status.load()) {

                case GameStatus::MENU:
                    renderer.renderMenu();
                    break;
                
                case GameStatus::INSTRUCTIONS: 
                    renderer.renderInstructions();
                    gs.status.store(GameStatus::MENU);
                    break;

                case GameStatus::SCORES: 
                    renderer.renderScores();
                    gs.status.store(GameStatus::MENU);
                    break;

                case GameStatus::RUNNING:
                    renderer.render(gs, *currentLevel_);
                    break;

                case GameStatus::PAUSED:
                    renderer.renderPause(gs);
                    break;

                case GameStatus::SHOP:
                    renderer.renderShop(gs);
                    break;

                case GameStatus::VICTORY:
                    renderer.renderVictory();
                    
                    if (ScoreManager::isHighScore(gs.score.load())) {
                        std::string name = "JUGADOR";
                        ScoreManager::save(name, gs.score.load(), gs.phase.load());
                    }
                    break;

                case GameStatus::GAME_OVER:
                    renderer.renderGameOver();
                    
                    if (ScoreManager::isHighScore(gs.score.load())) {
                        std::string name = "JUGADOR";
                        ScoreManager::save(name, gs.score.load(), gs.phase.load());
                    }
                    break;

                default:
                    renderer.render(gs, *currentLevel_);
                    break;
            }
        }

        // =====================
        // CONTROL DE TIEMPO
        // =====================
        std::this_thread::sleep_for(std::chrono::milliseconds(GAME_LOOP_MS));
    }
}