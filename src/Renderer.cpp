#include "Renderer.h"
#include "HUD.h"
#include <cstdio>
#include <cstring>

Renderer::Renderer() {
    clearBuffer();
}

void Renderer::render(const GameState& gs, const Level& level) {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(gs.renderMutex));

    clearBuffer();

    // Bordes de la pantalla
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        buffer_[y][0]              = CHAR_WALL;
        buffer_[y][SCREEN_WIDTH-1] = CHAR_WALL;
    }
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        buffer_[0][x]              = CHAR_WALL;
        buffer_[SCREEN_HEIGHT-1][x]= CHAR_WALL;
    }

    drawPlatforms(level.getPlatforms());

    {
        std::lock_guard<std::mutex> el(const_cast<std::mutex&>(gs.enemyMutex));
        drawEnemies(gs.enemies);
    }

    {
        std::lock_guard<std::mutex> pjl(const_cast<std::mutex&>(gs.playerProjMutex));
        drawProjectiles(gs.playerProjs, MAX_PLAYER_PROJ);
    }
    {
        std::lock_guard<std::mutex> ejl(const_cast<std::mutex&>(gs.enemyProjMutex));
        drawProjectiles(gs.enemyProjs, MAX_ENEMY_PROJ);
    }

    {
        std::lock_guard<std::mutex> pl(const_cast<std::mutex&>(gs.pitMutex));
        drawPlayer(gs.pit);
    }

    drawHUD(gs);
    flushBuffer();
}

// ─── Pantallas especiales ─────────────────────────────────────────────────────
void Renderer::renderMenu() {
    // Mover cursor al inicio y limpiar
    printf("\033[H\033[J");
    printf("╔══════════════════════════════════════╗\n");
    printf("║           KID ICARUS  (ASCII)        ║\n");
    printf("║                                      ║\n");
    printf("║   [1] Modo Manual (teclado)           ║\n");
    printf("║   [2] Modo Auto (CPU)                 ║\n");
    printf("║   [ESC] Salir                         ║\n");
    printf("╚══════════════════════════════════════╝\n");
    fflush(stdout);
}

void Renderer::renderShop(const GameState& gs) {
    printf("\033[H\033[J");
    int hearts;
    {
        std::lock_guard<std::mutex> pl(const_cast<std::mutex&>(gs.pitMutex));
        hearts = gs.pit.hearts;
    }
    printf("╔══════════════════════════════════════╗\n");
    printf("║              T I E N D A             ║\n");
    printf("║                                      ║\n");
    printf("║  Corazones: %-3d                     ║\n", hearts);
    printf("║                                      ║\n");
    printf("║  [C] Comprar +%d HP  (cuesta %d ♥)   ║\n", SHOP_HP_GAIN, SHOP_HEART_COST);
    printf("║  [Enter/Q] Continuar a Fase 3        ║\n");
    printf("╚══════════════════════════════════════╝\n");
    fflush(stdout);
}

void Renderer::renderPause(const GameState& gs) {
    HUDData hud = HUD::getSnapshot();
    printf("\033[H\033[J");
    printf("  *** PAUSADO ***\n");
    printf("  Fase: %d  |  Vidas: %d  |  HP: %d/%d  |  Corazones: %d\n",
            hud.phase, hud.lives, hud.hp, MAX_HP, hud.hearts);
    printf("  [P] Reanudar   [ESC] Salir\n");
    fflush(stdout);
}

void Renderer::renderVictory() {
    printf("\033[H\033[J");
    printf("╔══════════════════════════════════════╗\n");
    printf("║          ¡ V I C T O R I A !         ║\n");
    printf("║   Medusa ha sido derrotada.          ║\n");
    printf("║   Palutena está a salvo.             ║\n");
    printf("╚══════════════════════════════════════╝\n");
    fflush(stdout);
}

void Renderer::renderGameOver() {
    printf("\033[H\033[J");
    printf("╔══════════════════════════════════════╗\n");
    printf("║         G A M E  O V E R            ║\n");
    printf("╚══════════════════════════════════════╝\n");
    fflush(stdout);
}

// ─── Helpers de buffer ────────────────────────────────────────────────────────
void Renderer::clearBuffer() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        memset(buffer_[y], CHAR_EMPTY, SCREEN_WIDTH);
        buffer_[y][SCREEN_WIDTH] = '\0';
    }
}

void Renderer::put(int x, int y, char c) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        buffer_[y][x] = c;
}

void Renderer::drawPlatforms(const std::vector<Level::Platform>& plats) {
    for (const auto& p : plats)
        for (int i = 0; i < p.length; ++i)
            put(p.x + i, p.y, CHAR_PLATFORM);
}

void Renderer::drawPlayer(const Player& pit) {
    put(pit.pos.x, pit.pos.y, CHAR_PIT);
}

void Renderer::drawEnemies(const std::vector<Enemy>& enemies) {
    for (const auto& e : enemies) {
        if (!e.alive) continue;
        char c;
        switch (e.type) {
            case EnemyType::MONOEYE:  c = CHAR_MONOEYE; break;
            case EnemyType::SHEMUM:   c = CHAR_SHEMUM;  break;
            case EnemyType::REAPER:   c = CHAR_REAPER;  break;
            case EnemyType::REAPETTE: c = CHAR_REAPETTE;break;
            case EnemyType::MEDUSA:   c = CHAR_MEDUSA;  break;
            default:                  c = '?';
        }
        put(e.pos.x, e.pos.y, c);
    }
}

void Renderer::drawProjectiles(const Projectile* projs, int count) {
    for (int i = 0; i < count; ++i) {
        if (!projs[i].active) continue;
        // Proyectil de Pit va con '>' o '<'; enemigo con 'o'
        char c = (projs[i].dir == Direction::LEFT) ? '<' : CHAR_PROJ_PIT;
        put(projs[i].pos.x, projs[i].pos.y, c);
    }
}

void Renderer::drawHUD(const GameState& gs) {
    HUDData hud = HUD::getSnapshot();
    // HUD en la fila 0 (encima del borde), sobreescribe el borde superior
    // Formato: [♥♥♥] HP:10/10  ♡:5  F:2
    char hudLine[SCREEN_WIDTH + 1];
    snprintf(hudLine, sizeof(hudLine),
            " Vidas:%d HP:%02d/%02d Cor:%02d Fase:%d ",
            hud.lives, hud.hp, MAX_HP, hud.hearts, hud.phase);
    for (int i = 0; hudLine[i] && i < SCREEN_WIDTH; ++i)
        buffer_[0][i] = hudLine[i];
}

void Renderer::flushBuffer() {
    // Mover cursor al origen sin limpiar (evita parpadeo)
    printf("\033[H");
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        printf("%s\n", buffer_[y]);
    }
    fflush(stdout);
}
