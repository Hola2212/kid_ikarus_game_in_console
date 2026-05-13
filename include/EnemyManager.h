#pragma once
#include "GameState.h"
#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
// EnemyManager — Hilo dedicado al movimiento y comportamiento de enemigos.
//
// Responsabilidades:
//   • Mover Monoeye, Shemum, Reaper, Reapette y Medusa según sus patrones
//   • Invocar Reapettes cuando Reaper detecta a Pit
//   • Escalar velocidades según la fase actual
//   • Eliminar enemigos muertos de la lista
//
// Acceso a gs.enemies siempre bajo gs.enemyMutex.
// ─────────────────────────────────────────────────────────────────────────────
class EnemyManager {
public:
    static void threadFunc(GameState* gs, const Level* level);

private:
    static void tick(GameState& gs, const Level& level);

    static void moveMonoeye (Enemy& e, int phase);
    static void moveShemum  (Enemy& e, const std::vector<Level::Platform>& plats);
    static void moveReaper  (Enemy& e, const Player& pit, GameState& gs);
    static void moveReapette(Enemy& e, const Player& pit);
    static void moveMedusa  (Enemy& e, int tick);

    // Invoca Reapettes; llama bajo enemyMutex ya tomado
    static void spawnReapettes(Enemy& reaper, GameState& gs);

    static int speedForPhase(int phase);  // multiplicador de velocidad
};
