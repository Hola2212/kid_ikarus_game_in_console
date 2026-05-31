#include "Physics.h"

#include <thread>
#include <chrono>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Physics thread
// ─────────────────────────────────────────────────────────────────────────────

void Physics::threadFunc(
    GameState* gs,
    const Level* level
) {

    while (gs->running.load()) {

        // Solo procesar física durante gameplay
        if (gs->status.load() == GameStatus::RUNNING) {

            tick(*gs, *level);
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(PIT_THREAD_MS)
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Physics tick
// ─────────────────────────────────────────────────────────────────────────────

void Physics::tick(
    GameState& gs,
    const Level& level
) {

    std::lock_guard<std::mutex> lock(gs.pitMutex);

    Player& pit = gs.pit;

    // =========================================
    // Aplicar gravedad
    // =========================================

    applyGravity(pit);

    // =========================================
    // Calcular siguiente posición vertical
    // =========================================

    int nextY = pit.pos.y + pit.velY;

    // =========================================
    // Reset estado temporal
    // =========================================

    pit.onGround = false;

    // =========================================
    // Revisar plataformas (solo bajando)
    // Ignora dropFromY: la plataforma específica
    // que el jugador está atravesando.
    // =========================================

    if (pit.velY >= 0) {

        for (const auto& plat : level.getPlatforms()) {

            // Solo ignorar la plataforma de la que se bajó
            if (plat.y == pit.dropFromY)
                continue;

            bool inX =
                pit.pos.x >= plat.x &&
                pit.pos.x < plat.x + plat.length;

            if (!inX)
                continue;

            bool landsOnPlatform =
                pit.pos.y <= plat.y - 1 &&
                nextY     >= plat.y - 1;

            if (landsOnPlatform) {

                pit.pos.y  = plat.y - 1;
                pit.velY   = 0;
                pit.onGround = true;
                pit.dropFromY = -1;    // ya aterrizó, limpiar

                clampToBounds(pit);

                return;
            }
        }
    }

    // Las plataformas son de una sola dirección:
    // solo bloquean desde arriba (aterrizaje).
    // El jugador puede saltar a través de ellas desde abajo.

    // =========================================
    // Movimiento libre
    // =========================================

    pit.pos.y = nextY;

    // =========================================
    // Clamp horizontal
    // =========================================

    clampToBounds(pit);
}

// ─────────────────────────────────────────────────────────────────────────────
// Gravedad
// ─────────────────────────────────────────────────────────────────────────────

void Physics::applyGravity(Player& pit) {

    // Si está en el suelo:
    // no aplicar gravedad
    if (pit.onGround)
        return;

    // =========================================
    // Gravedad suave
    // =========================================

    pit.velY =
        std::min(
            pit.velY + 1,
            2
        );
}

// ─────────────────────────────────────────────────────────────────────────────
// Legacy helper
// (ya no se usa directamente,
// pero se mantiene por compatibilidad)
// ─────────────────────────────────────────────────────────────────────────────

bool Physics::isOnPlatform(
    const Player& pit,
    const std::vector<Level::Platform>& plats
) {

    for (const auto& plat : plats) {

        bool inX =
            pit.pos.x >= plat.x &&
            pit.pos.x < plat.x + plat.length;

        bool inY =
            pit.pos.y + 1 == plat.y;

        if (inX && inY)
            return true;
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Bounds
// ─────────────────────────────────────────────────────────────────────────────

void Physics::clampToBounds(Player& pit) {

    // Horizontal
    if (pit.pos.x < 0)
        pit.pos.x = 0;

    if (pit.pos.x >= SCREEN_WIDTH)
        pit.pos.x = SCREEN_WIDTH - 1;

    // Superior
    if (pit.pos.y < 0) {

        pit.pos.y = 0;

        pit.velY = 0;
    }

    // El límite inferior
    // lo maneja CollisionSystem
}