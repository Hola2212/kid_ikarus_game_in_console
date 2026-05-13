#pragma once
#include <thread>
#include <memory>
#include "GameState.h"
#include "Level.h"
#include "InputHandler.h"
#include "Renderer.h"

// ─────────────────────────────────────────────────────────────────────────────
// GameLoop — Orquesta el ciclo principal y los hilos secundarios.
// Responsabilidades:
//   1. Inicializar el GameState y el nivel de cada fase
//   2. Lanzar y detener los hilos (Pit, enemigos, proyectiles x2, HUD)
//   3. Ejecutar el bucle principal:
//        a) Procesar input
//        b) Detectar colisiones (CollisionSystem)
//        c) Verificar condición de victoria / derrota / transición de fase
//        d) Invocar Renderer para dibujar el frame
//        e) Dormir GAME_LOOP_MS
//   4. Gestionar la tienda entre fase 2 y fase 3
//   5. Lanzar la pelea con Medusa al terminar fase 3
// ─────────────────────────────────────────────────────────────────────────────
class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    // Punto de entrada: muestra menú y arranca el juego
    void run();

private:
    // ── Ciclo de un nivel completo (3 fases) ──────────────────────────────
    void runLevel();

    // ── Ciclo de una fase ─────────────────────────────────────────────────
    void runPhase(int phase);

    // ── Tienda entre fase 2 → 3 ───────────────────────────────────────────
    void runShop();

    // ── Pelea con Medusa ──────────────────────────────────────────────────
    void runBossFight();

    // ── Gestión de hilos ──────────────────────────────────────────────────
    void startThreads(const Level* level);
    void stopThreads();
    void joinThreads();

    // ── Un tick del bucle principal ───────────────────────────────────────
    void tick();

    // ── Verifica si todos los enemigos de la fase están muertos ──────────
    bool phaseCleared() const;

    // ── Miembros ──────────────────────────────────────────────────────────
    GameState       gs_;
    InputHandler    input_;
    Renderer        renderer_;
    std::unique_ptr<Level> currentLevel_;

    std::thread     pitThread_;
    std::thread     enemyThread_;
    std::thread     playerProjThread_;
    std::thread     enemyProjThread_;
    std::thread     hudThread_;
};
