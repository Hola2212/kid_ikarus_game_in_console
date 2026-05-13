#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
Level::Level(int phase) : phase_(phase) {
    switch (phase_) {
        case 1: buildPhase1(); break;
        case 2: buildPhase2(); break;
        case 3: buildPhase3(); break;
        default: buildPhase1(); break;
    }
}

// ─── Fase 1 — 3 Monoeye ───────────────────────────────────────────────────────
void Level::buildPhase1() {
    // Plataformas: {x_inicio, y, longitud}
    platforms_ = {
        {0,  SCREEN_HEIGHT - 1, SCREEN_WIDTH},  // suelo
        {3,  18, 8},
        {12, 15, 7},
        {22, 12, 9},
        {5,  9,  6},
        {16, 6,  8},
        {2,  3,  SCREEN_WIDTH - 4}              // techo / meta fase
    };
}

// ─── Fase 2 — 4 Monoeye + 2 Shemum ───────────────────────────────────────────
void Level::buildPhase2() {
    platforms_ = {
        {0,  SCREEN_HEIGHT - 1, SCREEN_WIDTH},
        {5,  17, 6},
        {14, 14, 5},
        {2,  11, 7},
        {18, 8,  8},
        {6,  5,  10},
        {1,  2,  SCREEN_WIDTH - 2}
    };
}

// ─── Fase 3 — 5 Monoeye + 3 Shemum + 1-2 Reaper ─────────────────────────────
void Level::buildPhase3() {
    platforms_ = {
        {0,  SCREEN_HEIGHT - 1, SCREEN_WIDTH},
        {4,  16, 5},
        {13, 13, 6},
        {1,  10, 8},
        {20, 7,  7},
        {8,  4,  9},
        {0,  1,  SCREEN_WIDTH}                  // arena de Medusa
    };
}

// ─────────────────────────────────────────────────────────────────────────────
std::vector<Enemy> Level::spawnEnemies() const {
    std::vector<Enemy> enemies;

    auto addEnemy = [&](EnemyType t, int x, int y, int hp = 1, int hearts = 1) {
        Enemy e;
        e.type          = t;
        e.pos           = {x, y};
        e.hp            = hp;
        e.alive         = true;
        e.heartsOnDeath = hearts;
        enemies.push_back(e);
    };

    if (phase_ >= 1) {
        // Monoeye fase 1: 3 unidades
        addEnemy(EnemyType::MONOEYE, 5,  16);
        addEnemy(EnemyType::MONOEYE, 20, 13);
        addEnemy(EnemyType::MONOEYE, 10, 8);
    }
    if (phase_ >= 2) {
        // Monoeye adicionales
        addEnemy(EnemyType::MONOEYE, 30, 10);
        // Shemum fase 2: 2 unidades
        addEnemy(EnemyType::SHEMUM, 8,  SCREEN_HEIGHT - 2);
        addEnemy(EnemyType::SHEMUM, 22, SCREEN_HEIGHT - 2);
    }
    if (phase_ >= 3) {
        // Monoeye adicional
        addEnemy(EnemyType::MONOEYE, 15, 5);
        // Shemum adicional
        addEnemy(EnemyType::SHEMUM, 12, SCREEN_HEIGHT - 2);
        // Reaper fase 3: 2 unidades
        addEnemy(EnemyType::REAPER, 6,  SCREEN_HEIGHT - 2, 3, 2);
        addEnemy(EnemyType::REAPER, 28, SCREEN_HEIGHT - 2, 3, 2);
    }

    return enemies;
}

// ─────────────────────────────────────────────────────────────────────────────
Enemy Level::spawnMedusa() const {
    Enemy medusa;
    medusa.type          = EnemyType::MEDUSA;
    medusa.pos           = {SCREEN_WIDTH / 2, 2};
    medusa.hp            = 10;
    medusa.alive         = true;
    medusa.heartsOnDeath = 0;
    return medusa;
}
