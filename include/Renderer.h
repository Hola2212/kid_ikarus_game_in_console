#pragma once
#include "GameState.h"
#include "Level.h"

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
    bool first_{true};

    void clear();
    void put(int x, int y, char c);
    void putStr(int x, int y, const char* s);
    void flush();
    void drawHUD(const GameState& gs);
};