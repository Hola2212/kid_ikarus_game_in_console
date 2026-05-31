#include "ProjectileManager.h"
#include <thread>
#include <chrono>
#include <cmath>

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
    if (!gs.playerProjSem.try_acquire()) return false;

    std::lock_guard<std::mutex> lock(gs.playerProjMutex);

    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
        if (!gs.playerProjs[i].active) {
            Player snapPit;
            {
                std::lock_guard<std::mutex> pl(gs.pitMutex);
                snapPit = gs.pit;
            }
            Direction finalDir = (dir == Direction::NONE) ? snapPit.facing : dir;
            Position start = snapPit.pos;
            switch (finalDir) {
                case Direction::RIGHT: start.x += 1; break;
                case Direction::LEFT:  start.x -= 1; break;
                case Direction::UP:    start.y -= 1; break;
                default: break;
            }
            gs.playerProjs[i].pos    = start;
            gs.playerProjs[i].dir    = finalDir;
            gs.playerProjs[i].active = true;
            return true;
        }
    }
    gs.playerProjSem.release();
    return false;
}

bool ProjectileManager::fireEnemyProjectile(GameState& gs, Position origin, int velX, int velY) {
    if (!gs.enemyProjSem.try_acquire()) return false;

    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
        if (!gs.enemyProjs[i].active) {
            gs.enemyProjs[i].pos    = origin;
            gs.enemyProjs[i].velX   = velX;
            gs.enemyProjs[i].velY   = velY;
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

        switch (p.dir) {
            case Direction::RIGHT: p.pos.x++; break;
            case Direction::LEFT:  p.pos.x--; break;
            case Direction::UP:    p.pos.y--; break;
            default: break;
        }

        bool outOfBounds = (p.pos.x < 0 || p.pos.x >= SCREEN_WIDTH ||
                            p.pos.y < 0 || p.pos.y >= GAME_HEIGHT);

        bool hitPlat = hitsPlatform(p, level.getPlatforms());

        bool hitEnemy = false;
        {
            std::lock_guard<std::mutex> el(gs.enemyMutex);
            for (auto& e : gs.enemies) {
                if (e.alive && hitsEnemy(p, e)) {
                    e.hp--;
                    if (e.hp <= 0) {
                        e.alive = false;
                        std::lock_guard<std::mutex> pl(gs.pitMutex);
                        gs.pit.hearts += e.heartsOnDeath;
                        // Fix puntaje: sumar 10 puntos por enemigo eliminado
                        gs.score.fetch_add(10);
                    }
                    hitEnemy = true;
                    break;
                }
            }
        }

        if (outOfBounds || hitPlat || hitEnemy) {
            p.active = false;
            gs.playerProjSem.release();
        }
    }
}

void ProjectileManager::tickEnemy(GameState& gs, const Level& level) {
    static int tick = 0;
    ++tick;

    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

    Player pitSnap;
    {
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        pitSnap = gs.pit;
    }

    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
        Projectile& p = gs.enemyProjs[i];
        if (!p.active) continue;

        // Movimiento horizontal cada tick, vertical cada 2 ticks
        p.pos.x += p.velX;
        if (tick % 2 == 0) p.pos.y += p.velY;

        bool outOfBounds = (p.pos.x < 0 || p.pos.x >= SCREEN_WIDTH ||
                            p.pos.y < 0 || p.pos.y >= GAME_HEIGHT);
        bool hitPlat = hitsPlatform(p, level.getPlatforms());
        bool hitPit  = hitsPlayer(p, pitSnap) && !pitSnap.crouching;

        if (hitPit) {
            std::lock_guard<std::mutex> pl(gs.pitMutex);
            // Fix daño: respetar invencibilidad
            if (gs.pit.invincibleTicks > 0) {
                gs.pit.invincibleTicks--;
            } else {
                gs.pit.hp--;
                gs.pit.invincibleTicks = 15;
                if (gs.pit.hp <= 0) {
                    gs.pit.lives--;
                    gs.pit.hp = MAX_HP;
                    if (gs.pit.lives <= 0) {
                        gs.status.store(GameStatus::GAME_OVER);
                    }
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
    return (std::abs(p.pos.x - pit.pos.x) <= 1 &&
            std::abs(p.pos.y - pit.pos.y) <= 2);
}

bool ProjectileManager::hitsEnemy(const Projectile& p, const Enemy& e) {
    return (p.pos.x == e.pos.x && p.pos.y == e.pos.y);
}