#include "ProjectileManager.h"
#include <thread>
#include <chrono>

// ─── Hilo proyectiles de Pit ──────────────────────────────────────────────────
void ProjectileManager::playerProjThread(GameState* gs, const Level* level) {
    while (gs->running.load()) {
        if (gs->status.load() == GameStatus::RUNNING) {
            tickPlayer(*gs, *level);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PROJ_THREAD_MS));
    }
}

// ─── Hilo proyectiles de enemigos ────────────────────────────────────────────
void ProjectileManager::enemyProjThread(GameState* gs, const Level* level) {
    while (gs->running.load()) {
        if (gs->status.load() == GameStatus::RUNNING) {
            tickEnemy(*gs, *level);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PROJ_THREAD_MS));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
bool ProjectileManager::firePlayerProjectile(GameState& gs, Direction dir) {
    // Intentamos adquirir un slot del semáforo (no bloqueante)
    if (!gs.playerProjSem.try_acquire()) return false;

    std::lock_guard<std::mutex> lock(gs.playerProjMutex);

    // Buscar slot libre
    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
        if (!gs.playerProjs[i].active) {
            Player snapPit;
            {
                std::lock_guard<std::mutex> pl(gs.pitMutex);
                snapPit = gs.pit;
            }
            gs.playerProjs[i].pos    = snapPit.pos;
            gs.playerProjs[i].dir    = (dir == Direction::NONE) ? snapPit.facing : dir;
            gs.playerProjs[i].active = true;
            return true;
        }
    }
    gs.playerProjSem.release(); // No se usó, devolver
    return false;
}

bool ProjectileManager::fireEnemyProjectile(GameState& gs, Position origin, Direction dir) {
    if (!gs.enemyProjSem.try_acquire()) return false;

    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
        if (!gs.enemyProjs[i].active) {
            gs.enemyProjs[i].pos    = origin;
            gs.enemyProjs[i].dir    = dir;
            gs.enemyProjs[i].active = true;
            return true;
        }
    }
    gs.enemyProjSem.release();
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
void ProjectileManager::tickPlayer(GameState& gs, const Level& level) {
    std::lock_guard<std::mutex> lock(gs.playerProjMutex);

    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
        Projectile& p = gs.playerProjs[i];
        if (!p.active) continue;

        // Mover proyectil
        switch (p.dir) {
            case Direction::RIGHT: p.pos.x++; break;
            case Direction::LEFT:  p.pos.x--; break;
            case Direction::UP:    p.pos.y--; break;
            default: break;
        }

        // ¿Fuera de pantalla?
        bool outOfBounds = (p.pos.x < 0 || p.pos.x >= SCREEN_WIDTH ||
                            p.pos.y < 0 || p.pos.y >= SCREEN_HEIGHT);

        // ¿Golpea plataforma?
        bool hitPlat = hitsPlatform(p, level.getPlatforms());

        // ¿Golpea enemigo?
        bool hitEnemy = false;
        {
            std::lock_guard<std::mutex> el(gs.enemyMutex);
            for (auto& e : gs.enemies) {
                if (e.alive && hitsEnemy(p, e)) {
                    e.hp--;
                    if (e.hp <= 0) {
                        e.alive = false;
                        // Dar corazones al jugador
                        std::lock_guard<std::mutex> pl(gs.pitMutex);
                        gs.pit.hearts += e.heartsOnDeath;
                    }
                    hitEnemy = true;
                    break;
                }
            }
        }

        if (outOfBounds || hitPlat || hitEnemy) {
            p.active = false;
            gs.playerProjSem.release();  // Liberar slot
        }
    }
}

void ProjectileManager::tickEnemy(GameState& gs, const Level& level) {
    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

    // Snapshot de Pit
    Player pitSnap;
    {
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        pitSnap = gs.pit;
    }

    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
        Projectile& p = gs.enemyProjs[i];
        if (!p.active) continue;

        switch (p.dir) {
            case Direction::RIGHT: p.pos.x++; break;
            case Direction::LEFT:  p.pos.x--; break;
            case Direction::UP:    p.pos.y--; break;
            default: break;
        }

        bool outOfBounds = (p.pos.x < 0 || p.pos.x >= SCREEN_WIDTH ||
                            p.pos.y < 0 || p.pos.y >= SCREEN_HEIGHT);
        bool hitPlat = hitsPlatform(p, level.getPlatforms());
        bool hitPit  = hitsPlayer(p, pitSnap) && !pitSnap.crouching;

        if (hitPit) {
            std::lock_guard<std::mutex> pl(gs.pitMutex);
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

        if (outOfBounds || hitPlat || hitPit) {
            p.active = false;
            gs.enemyProjSem.release();
        }
    }
}

// ─── Helpers de colisión ──────────────────────────────────────────────────────
bool ProjectileManager::hitsPlatform(const Projectile& p,
                                    const std::vector<Level::Platform>& plats) {
    for (const auto& plat : plats) {
        if (p.pos.y == plat.y &&
            p.pos.x >= plat.x && p.pos.x < plat.x + plat.length)
            return true;
    }
    return false;
}

bool ProjectileManager::hitsPlayer(const Projectile& p, const Player& pit) {
    return (p.pos.x == pit.pos.x && p.pos.y == pit.pos.y);
}

bool ProjectileManager::hitsEnemy(const Projectile& p, const Enemy& e) {
    return (p.pos.x == e.pos.x && p.pos.y == e.pos.y);
}
