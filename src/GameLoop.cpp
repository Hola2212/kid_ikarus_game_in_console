#include "GameLoop.h"

#include "ScoreManager.h"
#include "CollisionSystem.h"

#include <chrono>
#include <thread>

GameLoop::GameLoop() {

    running = true;

    currentLevel_ = std::make_unique<Level>(1);

    // =========================================
    // SPAWN INICIAL DE ENEMIGOS
    // =========================================

    gs.enemies = currentLevel_->spawnEnemies();

    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);

        gs.pit.pos.x = SCREEN_WIDTH / 2;
        gs.pit.pos.y = GAME_HEIGHT - 2;

        gs.pit.velY = 0;

        gs.pit.hp = MAX_HP;
        gs.pit.lives = MAX_LIVES;
        gs.pit.hearts = 0;

        gs.pit.onGround = true;
        gs.pit.crouching = false;

        gs.pit.facing = Direction::RIGHT;
    }

    gs.score.store(0);

    gs.phase.store(1);

    gs.running.store(true);

    gs.status.store(GameStatus::MENU);

    // =========================================
    // LANZAR THREADS
    // =========================================

    enemyThread_ = std::thread(
        EnemyManager::threadFunc,
        &gs,
        currentLevel_.get()
    );

    physicsThread_ = std::thread(
        Physics::threadFunc,
        &gs,
        currentLevel_.get()
    );

    projPitThread_ = std::thread(
        ProjectileManager::playerProjThread,
        &gs,
        currentLevel_.get()
    );

    projEnemyThread_ = std::thread(
        ProjectileManager::enemyProjThread,
        &gs,
        currentLevel_.get()
    );
}

GameLoop::~GameLoop() {

    // =========================================
    // DETENER THREADS
    // =========================================

    gs.running.store(false);

    // =========================================
    // JOIN THREADS
    // =========================================

    if (enemyThread_.joinable())
        enemyThread_.join();

    if (physicsThread_.joinable())
        physicsThread_.join();

    if (projPitThread_.joinable())
        projPitThread_.join();

    if (projEnemyThread_.joinable())
        projEnemyThread_.join();
}

// =====================================================
// RESET COMPLETO DEL JUEGO
// =====================================================

void GameLoop::resetGame() {

    // =========================================
    // RESET PLAYER
    // =========================================

    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);

        gs.pit.pos.x = SCREEN_WIDTH / 2;
        gs.pit.pos.y = GAME_HEIGHT - 2;

        gs.pit.velY = 0;

        gs.pit.hp = MAX_HP;
        gs.pit.lives = MAX_LIVES;
        gs.pit.hearts = 0;

        gs.pit.onGround = true;
        gs.pit.crouching = false;

        gs.pit.facing = Direction::RIGHT;
    }

    // =========================================
    // RESET SCORE / PHASE
    // =========================================

    gs.score.store(0);

    gs.phase.store(1);

    // =========================================
    // LIMPIAR ENEMIGOS
    // =========================================

    {
        std::lock_guard<std::mutex> lock(gs.enemyMutex);

        gs.enemies.clear();

        gs.enemies = currentLevel_->spawnEnemies();
    }

    // =========================================
    // LIMPIAR PROYECTILES JUGADOR
    // =========================================

    {
        std::lock_guard<std::mutex> lock(gs.playerProjMutex);

        for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {

            gs.playerProjs[i].active = false;
        }
    }

    // =========================================
    // LIMPIAR PROYECTILES ENEMIGOS
    // =========================================

    {
        std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

        for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {

            gs.enemyProjs[i].active = false;
        }
    }

    gs.status.store(GameStatus::RUNNING);
}

void GameLoop::run() {

    GameStatus lastRendered = static_cast<GameStatus>(-1);

    while (running && gs.running.load()) {

        // =====================
        // INPUT
        // =====================

        input.processInput(gs);

        // =========================================
        // RESTART COMPLETO
        // =========================================

        if (gs.status.load() == GameStatus::RUNNING &&
            gs.pit.lives == MAX_LIVES &&
            gs.score.load() == 0) {

            // Nada especial aquí,
            // reset se hace desde InputHandler
        }

        // =====================
        // UPDATE
        // =====================

        if (gs.status.load() == GameStatus::RUNNING) {

            // =========================================
            // SOLO COLISIONES
            // Física corre en Physics thread
            // =========================================

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