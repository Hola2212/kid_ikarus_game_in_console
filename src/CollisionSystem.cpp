#include "CollisionSystem.h"

void CollisionSystem::checkAll(GameState& gs, const Level& /*level*/) {
    checkPitVsEnemies(gs);
    checkPitVsEnemyProjs(gs);   // también manejado en ProjectileManager::tickEnemy
    checkPitOutOfBounds(gs);
}

// ─── Pit ↔ Enemigo (contacto directo) ────────────────────────────────────────
void CollisionSystem::checkPitVsEnemies(GameState& gs) {
    Player pitSnap;
    {
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        pitSnap = gs.pit;
    }

    std::lock_guard<std::mutex> el(gs.enemyMutex);
    for (auto& e : gs.enemies) {
        if (!e.alive) continue;
        if (e.pos.x == pitSnap.pos.x && e.pos.y == pitSnap.pos.y) {
            damagePlayer(gs, 1);
        }
    }
}

// ─── Pit ↔ Proyectil enemigo ──────────────────────────────────────────────────
// (Duplicado de seguridad; la lógica principal está en ProjectileManager)
void CollisionSystem::checkPitVsEnemyProjs(GameState& /*gs*/) {
    // Gestionado por ProjectileManager::tickEnemy
}

// ─── Flecha de Pit ↔ Enemigo ──────────────────────────────────────────────────
void CollisionSystem::checkPlayerProjVsEnemy(GameState& /*gs*/) {
    // Gestionado por ProjectileManager::tickPlayer
}

// ─── Pit cae por el fondo ─────────────────────────────────────────────────────
void CollisionSystem::checkPitOutOfBounds(GameState& gs) {
    std::lock_guard<std::mutex> pl(gs.pitMutex);
    if (gs.pit.pos.y >= SCREEN_HEIGHT) {
        gs.pit.pos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 2};  // Reaparecer
        gs.pit.velY = 0;
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
