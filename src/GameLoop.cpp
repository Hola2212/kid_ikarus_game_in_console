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

    hudThread_ = std::thread(
        HUD::threadFunc,
        &gs
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

    if (hudThread_.joinable())
        hudThread_.join();
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
        // RESTART — respawnear enemigos y reanudar
        // =========================================

        if (gs.restartRequested.load()) {

            gs.restartRequested.store(false);

            {
                std::lock_guard<std::mutex> lock(gs.enemyMutex);
                gs.enemies.clear();
                gs.enemies = currentLevel_->spawnEnemies();
            }

            gs.status.store(GameStatus::RUNNING);
        }

        // =====================
        // UPDATE
        // =====================

        if (gs.status.load() == GameStatus::RUNNING) {

            CollisionSystem::checkAll(gs, *currentLevel_);

            // =========================================
            // AVANCE DE FASE
            // =========================================

            bool atTop = false;
            {
                std::lock_guard<std::mutex> pl(gs.pitMutex);
                atTop = gs.pit.onGround && gs.pit.pos.y <= 1;
            }

            if (atTop) {

                int phase = gs.phase.load();

                if (phase == 1) {

                    gs.phase.store(2);
                    currentLevel_->rebuild(2);

                    {
                        std::lock_guard<std::mutex> el(gs.enemyMutex);
                        gs.enemies.clear();
                        gs.enemies = currentLevel_->spawnEnemies();
                    }

                    {
                        std::lock_guard<std::mutex> pl(gs.pitMutex);
                        gs.pit.pos        = {SCREEN_WIDTH / 2, GAME_HEIGHT - 2};
                        gs.pit.velY       = 0;
                        gs.pit.onGround   = true;
                        gs.pit.invincibleTicks = 15;
                    }

                    renderer.triggerFullRedraw();

                } else if (phase == 2) {

                    gs.status.store(GameStatus::SHOP);

                } else if (phase == 3) {

                    gs.status.store(GameStatus::VICTORY);
                }
            }
        }

        // =========================================
        // AVANCE DESDE TIENDA → FASE 3
        // =========================================

        if (gs.shopAdvanceRequested.load()) {

            gs.shopAdvanceRequested.store(false);

            gs.phase.store(3);
            currentLevel_->rebuild(3);

            {
                std::lock_guard<std::mutex> el(gs.enemyMutex);
                gs.enemies.clear();
                gs.enemies = currentLevel_->spawnEnemies();
            }

            {
                std::lock_guard<std::mutex> pl(gs.pitMutex);
                gs.pit.pos        = {SCREEN_WIDTH / 2, GAME_HEIGHT - 2};
                gs.pit.velY       = 0;
                gs.pit.onGround   = true;
                gs.pit.invincibleTicks = 15;
            }

            gs.status.store(GameStatus::RUNNING);
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

                    if (lastRendered != GameStatus::SHOP) {

                        renderer.renderShop(gs);

                        lastRendered = GameStatus::SHOP;
                    }

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