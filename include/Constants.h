#pragma once

// ─── Dimensiones de la pantalla ───────────────────────────────────────────────
constexpr int SCREEN_WIDTH  = 80;
constexpr int SCREEN_HEIGHT = 24;
constexpr int HUD_ROWS       = 3;
constexpr int GAME_ROW_START = HUD_ROWS + 1;
constexpr int GAME_HEIGHT    = SCREEN_HEIGHT - GAME_ROW_START;

// ─── Lógica de juego ──────────────────────────────────────────────────────────
constexpr int MAX_LIVES         = 3;
constexpr int MAX_HP            = 10;
constexpr int MAX_PLAYER_PROJ   = 3;
constexpr int MAX_ENEMY_PROJ    = 3;
constexpr int TOTAL_PHASES      = 3;    // Fases dentro del único nivel
constexpr int SHOP_HEART_COST   = 3;    // Corazones para comprar 1 HP
constexpr int SHOP_HP_GAIN      = 1;    // HP que da la tienda
constexpr int MAX_SCORES      = 5;

// ─── Velocidades (ms entre ticks) ─────────────────────────────────────────────
constexpr int GAME_LOOP_MS      = 100;  // ~10 FPS base
constexpr int PIT_THREAD_MS     = 80;
constexpr int ENEMY_THREAD_MS   = 120;
constexpr int PROJ_THREAD_MS    = 60;
constexpr int HUD_THREAD_MS     = 200;

// ─── ASCII art ────────────────────────────────────────────────────────────────
constexpr char CHAR_PIT         = '@';
constexpr char CHAR_MONOEYE     = 'M';
constexpr char CHAR_SHEMUM      = 'S';
constexpr char CHAR_REAPER      = 'R';
constexpr char CHAR_REAPETTE    = 'r';
constexpr char CHAR_MEDUSA      = 'G';
constexpr char CHAR_PROJ_PIT    = '>';
constexpr char CHAR_PROJ_ENEMY  = 'o';
constexpr char CHAR_PLATFORM    = '*';
constexpr char CHAR_SHOP        = '$';
constexpr char CHAR_EMPTY       = ' ';
constexpr char CHAR_WALL        = '#';
