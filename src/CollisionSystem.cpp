#include "CollisionSystem.h"

// ─── Entry point ──────────────────────────────────────────────────────────────
void CollisionSystem::checkAll(GameState& gs, const Level& /*level*/) {
    checkPitVsEnemies(gs);
    checkPitOutOfBounds(gs);
}

// ─── Pit ↔ Enemigo (contacto directo) ────────────────────────────────────────
void CollisionSystem::checkPitVsEnemies(GameState& gs) {
    // Snapshot de Pit sin mantener pitMutex mientras tomamos enemyMutex
    Player pitSnap;
    {
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        pitSnap = gs.pit;
    }

    // Buscar colisiones con enemigos
    std::vector<int> hitIndices;
    {
        std::lock_guard<std::mutex> el(gs.enemyMutex);
        for (int i = 0; i < (int)gs.enemies.size(); i++) {
            const auto& e = gs.enemies[i];
            if (!e.alive) continue;
            if (e.pos.x == pitSnap.pos.x && e.pos.y == pitSnap.pos.y) {
                hitIndices.push_back(i);
            }
        }
    }

    if (!hitIndices.empty()) {
        damagePlayer(gs, (int)hitIndices.size());
    }
}

// ─── Pit cae por el fondo de pantalla ─────────────────────────────────────────
void CollisionSystem::checkPitOutOfBounds(GameState& gs) {
    std::lock_guard<std::mutex> pl(gs.pitMutex);

    if (gs.pit.pos.y < SCREEN_HEIGHT) return;

    // Reaparecer en posición inicial
    gs.pit.pos      = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 2};
    gs.pit.velY     = 0;
    gs.pit.onGround = true;

    gs.pit.hp--;
    if (gs.pit.hp <= 0) {
        gs.pit.lives--;
        gs.pit.hp = MAX_HP;
        if (gs.pit.lives <= 0) {
            gs.status.store(GameStatus::GAME_OVER);
            gs.running.store(false);
        }
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
void CollisionSystem::damagePlayer(GameState& gs, int amount) {
    std::lock_guard<std::mutex> pl(gs.pitMutex);
    gs.pit.hp -= amount;
    if (gs.pit.hp <= 0) {
        gs.pit.lives--;
        gs.pit.hp = MAX_HP;
        if (gs.pit.lives <= 0) {
            gs.status.store(GameStatus::GAME_OVER);
            gs.running.store(false);
        }
    }
}

void CollisionSystem::damageEnemy(Enemy& e, int amount, GameState& gs) {
    e.hp -= amount;
    if (e.hp <= 0) {
        e.alive = false;
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        gs.pit.hearts += e.heartsOnDeath;
    }
}