#include "CpuPlayer.h"
#include "ProjectileManager.h"
#include "Constants.h"
#include <cmath>
#include <climits>
#include <algorithm>

void CpuPlayer::tick(GameState& gs, const Level& level) {

    static int jumpCooldown = 0;
    if (jumpCooldown > 0) jumpCooldown--;

    if (gs.status.load() != GameStatus::RUNNING) return;

    // ── Snapshot del jugador ──────────────────────────────────────────────────
    Player pit;
    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);
        pit = gs.pit;
    }

    auto platforms = level.getPlatforms();

    // ── Plataforma actual ─────────────────────────────────────────────────────
    const Level::Platform* curPlat = nullptr;
    for (const auto& plat : platforms) {
        if (pit.pos.y + 1 == plat.y &&
            pit.pos.x >= plat.x &&
            pit.pos.x <  plat.x + plat.length) {
            curPlat = &plat;
            break;
        }
    }

    bool onGroundFloor = curPlat && (curPlat->y == GAME_HEIGHT - 1);

    // ── Buscar enemigo más cercano ────────────────────────────────────────────
    Position targetPos{-1, -1};
    int minDist = INT_MAX;
    bool hayEnemigos = false;
    {
        std::lock_guard<std::mutex> lock(gs.enemyMutex);
        for (const auto& e : gs.enemies) {
            if (!e.alive) continue;
            hayEnemigos = true;
            int dist = std::abs(e.pos.x - pit.pos.x) +
                       std::abs(e.pos.y - pit.pos.y);
            if (dist < minDist) {
                minDist   = dist;
                targetPos = e.pos;
            }
        }
    }

    // ── Detectar proyectil enemigo en camino ──────────────────────────────────
    bool danger = false;
    {
        std::lock_guard<std::mutex> lock(gs.enemyProjMutex);
        for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
            const auto& p = gs.enemyProjs[i];
            if (!p.active) continue;
            if (std::abs(p.pos.y - pit.pos.y) > 2) continue;
            if (std::abs(p.pos.x - pit.pos.x) > 8) continue;
            bool left  = (p.velX < 0 && p.pos.x > pit.pos.x);
            bool right = (p.velX > 0 && p.pos.x < pit.pos.x);
            if (left || right) { danger = true; break; }
        }
    }

    // =========================================================================
    // MODO NAVEGACIÓN — sin enemigos, subir al tope para avanzar fase
    // =========================================================================
    if (!hayEnemigos) {

        // Siguiente plataforma objetivo: mayor y que esté por encima del jugador
        const Level::Platform* tgtPlat = nullptr;
        int bestY = -1;
        for (const auto& plat : platforms) {
            if (plat.y < pit.pos.y && plat.y > bestY) {
                bestY   = plat.y;
                tgtPlat = &plat;
            }
        }

        if (!tgtPlat) return;

        int tgtCenterX = tgtPlat->x + tgtPlat->length / 2;
        bool goRight   = tgtCenterX >= pit.pos.x;

        // Punto de salto:
        // - Suelo o en el aire → moverse al centro del objetivo y saltar en rango
        // - Plataforma regular → borde que apunta al objetivo
        int jumpFromX;
        if (!curPlat || onGroundFloor) {
            jumpFromX = tgtCenterX;
        } else {
            jumpFromX = goRight ? (curPlat->x + curPlat->length - 1)
                                : curPlat->x;
        }

        // Condición de salto: llegó AL borde (no "cerca", sino exactamente ahí)
        bool atJumpPos = goRight ? (pit.pos.x >= jumpFromX)
                                 : (pit.pos.x <= jumpFromX);

        {
            std::lock_guard<std::mutex> lock(gs.pitMutex);

            // Mover hacia el punto de salto
            int moveDx = jumpFromX - pit.pos.x;
            if (moveDx > 0) {
                gs.pit.pos.x = std::min(gs.pit.pos.x + 1, SCREEN_WIDTH - 1);
                gs.pit.facing = Direction::RIGHT;
            } else if (moveDx < 0) {
                gs.pit.pos.x = std::max(gs.pit.pos.x - 1, 0);
                gs.pit.facing = Direction::LEFT;
            }

            // Saltar cuando está en posición y el cooldown expiró
            if (gs.pit.onGround && atJumpPos && jumpCooldown == 0) {
                gs.pit.velY     = -4;
                gs.pit.onGround = false;
                jumpCooldown    = 10;
            }
        }

        return;
    }

    // =========================================================================
    // MODO COMBATE — hay enemigos, atacar
    // =========================================================================
    int dx = targetPos.x - pit.pos.x;
    int dy = targetPos.y - pit.pos.y;

    {
        std::lock_guard<std::mutex> lock(gs.pitMutex);

        if (std::abs(dx) > 1) {
            if (dx > 0) {
                gs.pit.pos.x = std::min(gs.pit.pos.x + 1, SCREEN_WIDTH - 1);
                gs.pit.facing = Direction::RIGHT;
            } else {
                gs.pit.pos.x = std::max(gs.pit.pos.x - 1, 0);
                gs.pit.facing = Direction::LEFT;
            }
        } else {
            gs.pit.facing = (dx >= 0) ? Direction::RIGHT : Direction::LEFT;
        }

        bool deberíaSaltar = danger || dy < -3;
        if (gs.pit.onGround && deberíaSaltar && jumpCooldown == 0) {
            gs.pit.velY     = -4;
            gs.pit.onGround = false;
            jumpCooldown    = 8;
        }
    }

    Direction dir;
    if (dy < -4 && std::abs(dy) > std::abs(dx))
        dir = Direction::UP;
    else
        dir = (dx >= 0) ? Direction::RIGHT : Direction::LEFT;

    ProjectileManager::firePlayerProjectile(gs, dir);
}
