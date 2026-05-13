#pragma once
#include <vector>
#include "GameState.h"
#include "Level.h"

// ─────────────────────────────────────────────────────────────────────────────
// SOLO un hilo llama a render() a la vez (protegido por gs.renderMutex).
// ─────────────────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer();

    // Dibuja un frame completo: plataformas + entidades + HUD
    void render(const GameState& gs, const Level& level);

    // Pantallas especiales
    void renderMenu();
    void renderShop(const GameState& gs);
    void renderVictory();
    void renderGameOver();
    void renderPause(const GameState& gs);

private:
    // Buffer doble para evitar parpadeo
    char buffer_[SCREEN_HEIGHT][SCREEN_WIDTH + 1];

    void clearBuffer();
    void drawPlatforms(const std::vector<Level::Platform>& plats);
    void drawPlayer(const Player& pit);
    void drawEnemies(const std::vector<Enemy>& enemies);
    void drawProjectiles(const Projectile* projs, int count);
    void drawHUD(const GameState& gs);
    void flushBuffer();

    // Escribe un carácter en el buffer (con bounds check)
    void put(int x, int y, char c);
};
