#include "GameLoop.h"
#include "Physics.h"
#include "EnemyManager.h"
#include "ProjectileManager.h"
#include "HUD.h"
#include "CollisionSystem.h"

#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
GameLoop::GameLoop() = default;
GameLoop::~GameLoop() { stopThreads(); joinThreads(); }

// ─────────────────────────────────────────────────────────────────────────────
void GameLoop::run() {
    renderer_.renderMenu();

    // Leer opción de menú (bloqueante simple)
    char c = 0;
    while (c != '1' && c != '2' && c != 27) {
        read(STDIN_FILENO, &c, 1);
    }
    if (c == 27) return;  // ESC → salir

    // Modo 2 (CPU) se puede implementar después; por ahora solo modo 1
    gs_.running.store(true);
    runLevel();

    // Pantalla final
    if (gs_.status.load() == GameStatus::VICTORY)
        renderer_.renderVictory();
    else
        renderer_.renderGameOver();

    std::this_thread::sleep_for(std::chrono::seconds(3));
}

// ─────────────────────────────────────────────────────────────────────────────
void GameLoop::runLevel() {
    // Reiniciar jugador
    {
        std::lock_guard<std::mutex> pl(gs_.pitMutex);
        gs_.pit = Player{};
    }

    for (int phase = 1; phase <= TOTAL_PHASES; ++phase) {
        if (!gs_.running.load()) return;

        // ── Tienda antes de la fase 3 ──────────────────────────────────────
        if (phase == 3) {
            runShop();
            if (!gs_.running.load()) return;
        }

        runPhase(phase);

        if (gs_.status.load() == GameStatus::GAME_OVER) return;
    }

    // ── Jefe final: Medusa ────────────────────────────────────────────────
    if (gs_.running.load()) runBossFight();
}

// ─────────────────────────────────────────────────────────────────────────────
void GameLoop::runPhase(int phase) {
    gs_.phase.store(phase);
    gs_.status.store(GameStatus::RUNNING);

    // Construir nivel y poblar enemigos
    currentLevel_ = std::make_unique<Level>(phase);
    {
        std::lock_guard<std::mutex> el(gs_.enemyMutex);
        gs_.enemies = currentLevel_->spawnEnemies();
    }

    // Reposicionar a Pit en la base
    {
        std::lock_guard<std::mutex> pl(gs_.pitMutex);
        gs_.pit.pos     = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 2};
        gs_.pit.velY    = 0;
        gs_.pit.onGround= true;
    }

    // Lanzar hilos
    startThreads(currentLevel_.get());

    // ── Bucle de la fase ──────────────────────────────────────────────────
    while (gs_.running.load()) {
        GameStatus status = gs_.status.load();

        if (status == GameStatus::PAUSED) {
            renderer_.renderPause(gs_);
            std::this_thread::sleep_for(std::chrono::milliseconds(GAME_LOOP_MS));
            continue;
        }
        if (status == GameStatus::GAME_OVER) break;

        // Procesar input
        input_.processInput(gs_);

        // Detectar colisiones
        CollisionSystem::checkAll(gs_, *currentLevel_);

        // ¿Fase completada?
        if (phaseCleared()) break;

        // Renderizar
        renderer_.render(gs_, *currentLevel_);

        std::this_thread::sleep_for(std::chrono::milliseconds(GAME_LOOP_MS));
    }

    stopThreads();
    joinThreads();
}

// ─────────────────────────────────────────────────────────────────────────────
void GameLoop::runShop() {
    gs_.status.store(GameStatus::SHOP);

    renderer_.renderShop(gs_);

    char c = 0;
    while (true) {
        read(STDIN_FILENO, &c, 1);

        if (c == 'c' || c == 'C') {
            std::lock_guard<std::mutex> pl(gs_.pitMutex);
            if (gs_.pit.hearts >= SHOP_HEART_COST &&
                gs_.pit.hp < MAX_HP) {
                gs_.pit.hearts -= SHOP_HEART_COST;
                gs_.pit.hp     += SHOP_HP_GAIN;
                if (gs_.pit.hp > MAX_HP) gs_.pit.hp = MAX_HP;
            }
            renderer_.renderShop(gs_);  // Actualizar pantalla
        }
        else if (c == '\n' || c == 'q' || c == 'Q' || c == 27) {
            break;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void GameLoop::runBossFight() {
    gs_.status.store(GameStatus::BOSS);

    // Agregar Medusa a la lista de enemigos
    {
        std::lock_guard<std::mutex> el(gs_.enemyMutex);
        gs_.enemies.clear();
        gs_.enemies.push_back(currentLevel_->spawnMedusa());
    }

    // Relanzar hilos con el mismo nivel (fase 3)
    startThreads(currentLevel_.get());

    while (gs_.running.load()) {
        input_.processInput(gs_);
        CollisionSystem::checkAll(gs_, *currentLevel_);
        renderer_.render(gs_, *currentLevel_);

        // ¿Medusa muerta?
        {
            std::lock_guard<std::mutex> el(gs_.enemyMutex);
            bool medusaDead = true;
            for (const auto& e : gs_.enemies)
                if (e.type == EnemyType::MEDUSA && e.alive)
                    medusaDead = false;
            if (medusaDead) {
                gs_.status.store(GameStatus::VICTORY);
                gs_.running.store(false);
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(GAME_LOOP_MS));
    }

    stopThreads();
    joinThreads();
}

// ─── Gestión de hilos ─────────────────────────────────────────────────────────
void GameLoop::startThreads(const Level* level) {
    gs_.running.store(true);

    pitThread_        = std::thread(Physics::threadFunc,                   &gs_, level);
    enemyThread_      = std::thread(EnemyManager::threadFunc,              &gs_, level);
    playerProjThread_ = std::thread(ProjectileManager::playerProjThread,   &gs_, level);
    enemyProjThread_  = std::thread(ProjectileManager::enemyProjThread,    &gs_, level);
    hudThread_        = std::thread(HUD::threadFunc,                       &gs_);
}

void GameLoop::stopThreads() {
    gs_.running.store(false);
}

void GameLoop::joinThreads() {
    if (pitThread_.joinable())        pitThread_.join();
    if (enemyThread_.joinable())      enemyThread_.join();
    if (playerProjThread_.joinable()) playerProjThread_.join();
    if (enemyProjThread_.joinable())  enemyProjThread_.join();
    if (hudThread_.joinable())        hudThread_.join();
}

// ─────────────────────────────────────────────────────────────────────────────
bool GameLoop::phaseCleared() const {
    std::lock_guard<std::mutex> el(const_cast<std::mutex&>(gs_.enemyMutex));
    for (const auto& e : gs_.enemies)
        if (e.alive && e.type != EnemyType::MEDUSA)
            return false;
    return true;
}
