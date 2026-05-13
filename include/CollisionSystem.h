#pragma once
#include "GameState.h"
#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
// CollisionSystem — Detecta y resuelve colisiones entre entidades.
//
// Llamado desde el game loop principal (no tiene hilo propio).
// Trabaja SOLO con locks ya tomados por quien llame.
//
// Tipos de colisión cubiertos:
//   • Pit ↔ Enemigo       → daño a Pit
//   • Pit ↔ Proyectil     → daño a Pit
//   • Flecha  ↔ Enemigo   → daño al enemigo, suelta corazón
//   • Proyectil ↔ Pared   → desactivar proyectil
//   • Pit cae de pantalla → perder vida
// ─────────────────────────────────────────────────────────────────────────────
class CollisionSystem {
public:
    // Evalúa todas las colisiones y modifica el GameState en consecuencia.
    // Debe llamarse con todos los mutexes relevantes TOMADOS.
    static void checkAll(GameState& gs, const Level& level);

private:
    static void checkPitVsEnemies      (GameState& gs);
    static void checkPitVsEnemyProjs   (GameState& gs);
    static void checkPlayerProjVsEnemy (GameState& gs);
    static void checkPitOutOfBounds    (GameState& gs);

    static void damagePlayer(GameState& gs, int amount);
    static void damageEnemy (Enemy& e, int amount, GameState& gs);
};
