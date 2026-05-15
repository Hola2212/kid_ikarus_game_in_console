#pragma once
#include <atomic>
#include <mutex>
#include <semaphore>
#include <vector>
#include "Constants.h"

// ─── Enumeraciones ────────────────────────────────────────────────────────────
enum class GameStatus {
    MENU,
    RUNNING,
    PAUSED,
    SHOP,       // Tienda entre fase 2 → fase 3
    BOSS,       // Pelea con Medusa
    VICTORY,
    GAME_OVER
};

enum class Direction { LEFT, RIGHT, UP, NONE };

enum class EnemyType { MONOEYE, SHEMUM, REAPER, REAPETTE, MEDUSA };

// ─── Estructuras de datos ─────────────────────────────────────────────────────
struct Position {
    int x{0};
    int y{0};
};

struct Projectile {
    Position  pos;
    Direction dir{Direction::RIGHT};
    bool      active{false};
};

struct Enemy {
    EnemyType type;
    Position  pos;
    int       hp{1};
    bool      alive{true};
    Direction dir{Direction::LEFT};
    int       heartsOnDeath{1};
};

struct Player {
    Position  pos{SCREEN_WIDTH / 2, SCREEN_HEIGHT - 2};
    int       lives{MAX_LIVES};
    int       hp{MAX_HP};
    int       hearts{0};         // moneda
    Direction facing{Direction::RIGHT};
    bool      crouching{false};
    bool      onGround{true};
    int       velY{0};           // velocidad vertical para gravedad/salto
    int invincibleTicks{0};  // ticks de invencibilidad tras recibir daño
};

// ─── Estado global compartido ─────────────────────────────────────────────────
struct GameState {
    // ── Estado principal ──────────────────────────────────────────────────
    std::atomic<GameStatus> status{GameStatus::MENU};
    std::atomic<int>        phase{1};           // Fase actual (1-3)
    std::atomic<bool>       running{false};     // Game loop activo

    // ── Jugador ───────────────────────────────────────────────────────────
    Player      pit;
    std::mutex  pitMutex;   // Protege toda la struct Player

    // ── Enemigos ──────────────────────────────────────────────────────────
    std::vector<Enemy> enemies;
    std::mutex         enemyMutex;

    // ── Proyectiles del jugador ────────────────────────────────────────────
    Projectile         playerProjs[MAX_PLAYER_PROJ];
    std::mutex         playerProjMutex;
    // Semáforo: limita proyectiles activos simultáneos
    std::counting_semaphore<MAX_PLAYER_PROJ> playerProjSem{MAX_PLAYER_PROJ};

    // ── Proyectiles de enemigos ────────────────────────────────────────────
    Projectile         enemyProjs[MAX_ENEMY_PROJ];
    std::mutex         enemyProjMutex;
    std::counting_semaphore<MAX_ENEMY_PROJ> enemyProjSem{MAX_ENEMY_PROJ};

    // ── Tablero (solo lectura para renderizado) ────────────────────────────
    std::mutex renderMutex;  // Un solo hilo dibuja a la vez

    // ── HUD ───────────────────────────────────────────────────────────────
    std::mutex hudMutex;
};
