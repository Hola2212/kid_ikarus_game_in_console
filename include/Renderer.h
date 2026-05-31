#pragma once
#include "GameState.h"
#include "Level.h"


enum class SpriteFrame {
    IDLE = 0,
    WALK1,
    WALK2,
    JUMP,
    DAMAGE,
    VICTORY,
    FRAME_COUNT
};

class Renderer {
public:
    Renderer();
    void render(const GameState& gs, const Level& level);
    void renderMenu();
    void renderInstructions();
    void renderScores();
    void renderShop(const GameState& gs);
    void renderPause(const GameState& gs);
    void renderVictory();
    void renderGameOver();

private:
    char back_[SCREEN_HEIGHT][SCREEN_WIDTH + 1];
    char front_[SCREEN_HEIGHT][SCREEN_WIDTH + 1];
    int  frame_{0};
    bool fullRedraw_{true};

    // Para detección de IDLE sin tocar lógica
    int  prevPitX_{-1};
    int  prevPitY_{-1};
    int prevDrawPitX_{-100};
    int prevDrawPitY_{-100};
    bool pitMoving_{false};
    // Para detectar transición PAUSED → RUNNING y limpiar overlay
    GameStatus prevStatus_{GameStatus::MENU};

    void clear();
    void put(int x, int y, char c);
    void putStr(int x, int y, const char* s);
    void flush();
    void drawHUD(const GameState& gs);
    void drawBackground(int phase, GameStatus status);
    void drawPitSprite(int x, int y, SpriteFrame f, bool facingLeft);
    void drawEnemySprite(int x, int y, EnemyType type, Direction dir);
    SpriteFrame getPlayerFrame(const Player& pit) const;

    // Sprites: [frame][fila], 4 filas × 6 frames
    static const char* const PIT_SPRITES[static_cast<int>(SpriteFrame::FRAME_COUNT)][4];
    // Enemigos: [tipo][fila], 3 filas × 5 tipos
    static const char* const ENEMY_SPRITES[5][3];
};