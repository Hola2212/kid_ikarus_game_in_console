#pragma once
#include "GameState.h"
#include "Level.h"

// Frames de animación de Pit
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
    void triggerFullRedraw() { fullRedraw_ = true; }
    void renderMenu();
    void renderInstructions();
    void renderScores();
    void renderShop(const GameState& gs);
    void renderPause(const GameState& gs);
    void renderVictory();
    void renderGameOver();

private:
    int  frame_{0};
    bool fullRedraw_{true};

    // Detección de movimiento para IDLE vs WALK
    int  prevPitX_{-1};
    int  prevPitY_{-1};
    bool pitMoving_{false};

    // Detección de transición PAUSED→RUNNING para limpiar overlay
    GameStatus prevStatus_{GameStatus::MENU};

    // Posición anterior de Pit para borrar sprite previo
    int  prevDrawPitX_{-99};
    int  prevDrawPitY_{-99};

    // Posiciones anteriores de enemigos para borrar sprites previos
    // MAX 20 enemigos simultáneos
    static constexpr int MAX_TRACKED_ENEMIES = 20;
    int prevEnemyX_[MAX_TRACKED_ENEMIES];
    int prevEnemyY_[MAX_TRACKED_ENEMIES];
    bool prevEnemyActive_[MAX_TRACKED_ENEMIES];

    // Posiciones anteriores de proyectiles
    int prevProjX_[MAX_PLAYER_PROJ];
    int prevProjY_[MAX_PLAYER_PROJ];
    bool prevProjActive_[MAX_PLAYER_PROJ];
    int prevEProjX_[MAX_ENEMY_PROJ];
    int prevEProjY_[MAX_ENEMY_PROJ];
    bool prevEProjActive_[MAX_ENEMY_PROJ];

    void drawHUD(const GameState& gs);
    void drawBackground(int phase, GameStatus status);
    void drawPitSprite(int x, int y, SpriteFrame f, bool facingLeft);
    void drawEnemySprite(int x, int y, EnemyType type, Direction dir);
    void clearSprite(int x, int y, int cols, int rows);
    SpriteFrame getPlayerFrame(const Player& pit) const;

    // Sprite sheets
    static const char* const PIT_SPRITES[static_cast<int>(SpriteFrame::FRAME_COUNT)][4];
    static const char* const ENEMY_SPRITES[5][3];
};