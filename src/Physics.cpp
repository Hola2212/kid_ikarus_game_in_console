#include "Physics.h"
#include <thread>
#include <chrono>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
void Physics::threadFunc(GameState* gs, const Level* level) {
    while (gs->running.load()) {
        // Solo procesa física cuando el juego está corriendo
        if (gs->status.load() == GameStatus::RUNNING) {
            tick(*gs, *level);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PIT_THREAD_MS));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void Physics::tick(GameState& gs, const Level& level) {
    std::lock_guard<std::mutex> lock(gs.pitMutex);
    Player& pit = gs.pit;

    // ── Gravedad ──────────────────────────────────────────────────────────
    applyGravity(pit);

    // ── Mover verticalmente ───────────────────────────────────────────────
    pit.pos.y += pit.velY;

    // ── Colisión con plataformas ──────────────────────────────────────────
    if (isOnPlatform(pit, level.getPlatforms())) {
        // Alinear con la superficie de la plataforma
        for (const auto& plat : level.getPlatforms()) {
            if (pit.pos.x >= plat.x && pit.pos.x < plat.x + plat.length) {
                if (pit.pos.y >= plat.y && pit.velY >= 0) {
                    pit.pos.y  = plat.y - 1;  // posicionar encima
                    pit.velY   = 0;
                    pit.onGround = true;
                    break;
                }
            }
        }
    } else {
        pit.onGround = false;
    }

    // ── Límites de pantalla (horizontal) ──────────────────────────────────
    clampToBounds(pit);
}

// ─────────────────────────────────────────────────────────────────────────────
void Physics::applyGravity(Player& pit) {
    if (!pit.onGround) {
        pit.velY = std::min(pit.velY + 1, 3);  // caída máx 3 celdas/tick
    }
}

bool Physics::isOnPlatform(const Player& pit, const std::vector<Level::Platform>& plats) {
    for (const auto& plat : plats) {
        bool inX = (pit.pos.x >= plat.x) && (pit.pos.x < plat.x + plat.length);
        bool inY = (pit.pos.y + 1 == plat.y) || (pit.pos.y == plat.y);
        if (inX && inY && pit.velY >= 0) return true;
    }
    return false;
}

void Physics::clampToBounds(Player& pit) {
    if (pit.pos.x < 0)                 pit.pos.x = 0;
    if (pit.pos.x >= SCREEN_WIDTH)     pit.pos.x = SCREEN_WIDTH - 1;
    // Caída por el fondo se detecta en CollisionSystem (pierde vida)
}
