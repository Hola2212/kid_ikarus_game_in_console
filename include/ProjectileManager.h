#pragma once
#include "GameState.h"
#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
// ProjectileManager — DOS hilos independientes:
//   • playerProjThread : mueve flechas de Pit, detecta colisión con enemigos
//   • enemyProjThread  : mueve proyectiles enemigos, detecta colisión con Pit
// ─────────────────────────────────────────────────────────────────────────────
class ProjectileManager {
public:
    // ── Hilo de proyectiles de Pit ─────────────────────────────────────────
    static void playerProjThread(GameState* gs, const Level* level);

    // ── Hilo de proyectiles enemigos ───────────────────────────────────────
    static void enemyProjThread(GameState* gs, const Level* level);

    // ── Disparo desde InputHandler (llama al semáforo + mutex) ────────────
    static bool firePlayerProjectile(GameState& gs, Direction dir);

    // ── Disparo desde EnemyManager ────────────────────────────────────────
    static bool fireEnemyProjectile(GameState& gs, Position origin, Direction dir);

private:
    static void tickPlayer(GameState& gs, const Level& level);
    static void tickEnemy (GameState& gs, const Level& level);

    static bool hitsPlatform(const Projectile& p, const std::vector<Level::Platform>& plats);
    static bool hitsPlayer  (const Projectile& p, const Player& pit);
    static bool hitsEnemy   (const Projectile& p, const Enemy& e);
};
