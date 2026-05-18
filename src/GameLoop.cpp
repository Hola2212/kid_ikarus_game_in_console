#include "GameLoop.h"
#include "ScoreManager.h"
#include "CollisionSystem.h"

#include <chrono>
#include <thread>

GameLoop::GameLoop() {

    running = true;

    currentLevel_ = std::make_unique<Level>(1);

    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);

        gs.pit.pos.x = SCREEN_WIDTH / 2;
        gs.pit.pos.y = GAME_HEIGHT - 2;

        gs.pit.velY = 0;

        gs.pit.hp = MAX_HP;
        gs.pit.lives = MAX_LIVES;

        gs.pit.onGround = true;
    }

    gs.running.store(true);
    gs.status.store(GameStatus::MENU);
}

void GameLoop::run() {

    GameStatus lastRendered = static_cast<GameStatus>(-1);

    while (running && gs.running.load()) {

        // =====================
        // INPUT
        // =====================

        input.processInput(gs);

        // =====================
        // UPDATE
        // =====================

        if (gs.status.load() == GameStatus::RUNNING) {

            {
                std::lock_guard<std::mutex> lock(gs.pitMutex);

                // gravedad
                if (!gs.pit.onGround) {
                    gs.pit.velY += 1;
                    gs.pit.pos.y += gs.pit.velY;
                }

                // suelo
                if (gs.pit.pos.y >= GAME_HEIGHT - 2) {

                    gs.pit.pos.y = GAME_HEIGHT - 2;

                    gs.pit.velY = 0;

                    gs.pit.onGround = true;
                }

                // techo
                if (gs.pit.pos.y < 0) {

                    gs.pit.pos.y = 0;

                    gs.pit.velY = 0;
                }
            }

            CollisionSystem::checkAll(gs, *currentLevel_);
        }

        // =====================
        // RENDER
        // =====================

        {
            std::lock_guard<std::mutex> lock(gs.renderMutex);

            GameStatus state = gs.status.load();

            switch (state) {

                case GameStatus::MENU:

                    if (lastRendered != GameStatus::MENU) {
                        renderer.renderMenu();
                        lastRendered = GameStatus::MENU;
                    }

                    break;

                case GameStatus::INSTRUCTIONS:

                    if (lastRendered != GameStatus::INSTRUCTIONS) {
                        renderer.renderInstructions();
                        lastRendered = GameStatus::INSTRUCTIONS;
                    }

                    break;

                case GameStatus::SCORES:

                    if (lastRendered != GameStatus::SCORES) {
                        renderer.renderScores();
                        lastRendered = GameStatus::SCORES;
                    }

                    break;

                case GameStatus::RUNNING:

                    lastRendered = GameStatus::RUNNING;

                    renderer.render(gs, *currentLevel_);

                    break;

                case GameStatus::PAUSED:

                    renderer.renderPause(gs);

                    break;

                case GameStatus::SHOP:

                    renderer.renderShop(gs);

                    break;

                case GameStatus::VICTORY:

                    if (lastRendered != GameStatus::VICTORY) {

                        renderer.renderVictory();

                        if (ScoreManager::isHighScore(gs.score.load())) {

                            std::string name = "JUGADOR";

                            ScoreManager::save(
                                name,
                                gs.score.load(),
                                gs.phase.load()
                            );
                        }

                        lastRendered = GameStatus::VICTORY;
                    }

                    break;

                case GameStatus::GAME_OVER:

                    if (lastRendered != GameStatus::GAME_OVER) {

                        renderer.renderGameOver();

                        if (ScoreManager::isHighScore(gs.score.load())) {

                            std::string name = "JUGADOR";

                            ScoreManager::save(
                                name,
                                gs.score.load(),
                                gs.phase.load()
                            );
                        }

                        lastRendered = GameStatus::GAME_OVER;
                    }

                    break;

                default:

                    renderer.render(gs, *currentLevel_);

                    break;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(GAME_LOOP_MS)
        );
    }
}