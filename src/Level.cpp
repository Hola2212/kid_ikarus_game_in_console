#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
Level::Level(int phase) : phase_(phase) {

    switch (phase_) {

        case 1:

            buildPhase1();

            break;

        case 2:

            buildPhase2();

            break;

        case 3:

            buildPhase3();

            break;

        default:

            buildPhase1();

            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FASE 1
// ─────────────────────────────────────────────────────────────────────────────

void Level::buildPhase1() {

    // Coordenadas RELATIVAS al área de juego
    // {x_inicio, y, longitud}

    platforms_ = {

        // suelo
        {0, GAME_HEIGHT - 1, SCREEN_WIDTH},

        {3,  16, 10},
        {11, 13, 10},
        {20, 10, 10},
        {8,  7,  12},
        {15, 4,  10},

        // techo/meta
        {23, 2, SCREEN_WIDTH - 8}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// FASE 2
// ─────────────────────────────────────────────────────────────────────────────

void Level::buildPhase2() {

    platforms_ = {

        {0, GAME_HEIGHT - 1, SCREEN_WIDTH},

        {5,  15, 6},
        {14, 12, 5},
        {2,  9,  7},
        {18, 6,  8},
        {6,  3,  10},

        {1, 1, SCREEN_WIDTH - 2}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// FASE 3
// ─────────────────────────────────────────────────────────────────────────────

void Level::buildPhase3() {

    platforms_ = {

        {0, GAME_HEIGHT - 1, SCREEN_WIDTH},

        {4,  14, 5},
        {13, 11, 6},
        {1,  8,  8},
        {20, 5,  7},
        {8,  2,  9},

        // arena Medusa
        {0, 1, SCREEN_WIDTH}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// SPAWN ENEMIGOS
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Enemy> Level::spawnEnemies() const {

    std::vector<Enemy> enemies;

    auto addEnemy =
        [&](EnemyType t, int x, int y, int hp = 1, int hearts = 1) {

            Enemy e;

            e.type          = t;

            e.pos           = {x, y};

            e.hp            = hp;

            e.alive         = true;

            e.heartsOnDeath = hearts;

            e.dir           = Direction::RIGHT;

            enemies.push_back(e);
        };

    // ─────────────────────────
    // FASE 1
    // ─────────────────────────

    if (phase_ >= 1) {

        addEnemy(EnemyType::MONOEYE, 5,  14);

        addEnemy(EnemyType::MONOEYE, 20, 11);

        addEnemy(EnemyType::MONOEYE, 10, 6);
    }

    // ─────────────────────────
    // FASE 2
    // ─────────────────────────

    if (phase_ >= 2) {

        addEnemy(EnemyType::MONOEYE, 30, 8);

        addEnemy(
            EnemyType::SHEMUM,
            8,
            GAME_HEIGHT - 2
        );

        addEnemy(
            EnemyType::SHEMUM,
            22,
            GAME_HEIGHT - 2
        );
    }

    // ─────────────────────────
    // FASE 3
    // ─────────────────────────

    if (phase_ >= 3) {

        addEnemy(EnemyType::MONOEYE, 15, 3);

        addEnemy(
            EnemyType::SHEMUM,
            12,
            GAME_HEIGHT - 2
        );

        addEnemy(
            EnemyType::REAPER,
            6,
            GAME_HEIGHT - 2,
            3,
            2
        );

        addEnemy(
            EnemyType::REAPER,
            28,
            GAME_HEIGHT - 2,
            3,
            2
        );
    }

    return enemies;
}

// ─────────────────────────────────────────────────────────────────────────────
// MEDUSA
// ─────────────────────────────────────────────────────────────────────────────

Enemy Level::spawnMedusa() const {

    Enemy medusa;

    medusa.type = EnemyType::MEDUSA;

    medusa.pos = {
        SCREEN_WIDTH / 2,
        2
    };

    medusa.hp = 10;

    medusa.alive = true;

    medusa.heartsOnDeath = 0;

    medusa.dir = Direction::LEFT;

    return medusa;
}