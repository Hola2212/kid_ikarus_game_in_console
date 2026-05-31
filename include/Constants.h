#pragma once

// Dimensiones de la pantalla 
constexpr int SCREEN_WIDTH  = 80;
constexpr int SCREEN_HEIGHT = 24;
constexpr int HUD_ROWS       = 3;
constexpr int GAME_ROW_START = HUD_ROWS + 1;
constexpr int GAME_HEIGHT    = SCREEN_HEIGHT - GAME_ROW_START;

// Lógica de juego
constexpr int MAX_LIVES         = 3;
constexpr int MAX_HP            = 10;
constexpr int MAX_PLAYER_PROJ   = 3;
constexpr int MAX_ENEMY_PROJ    = 3;
constexpr int TOTAL_PHASES      = 3;
constexpr int SHOP_HEART_COST   = 3;
constexpr int SHOP_HP_GAIN      = 1;
constexpr int MAX_SCORES        = 5;

// Velocidades (ms entre ticks)
constexpr int GAME_LOOP_MS      = 100;
constexpr int PIT_THREAD_MS     = 80;
constexpr int ENEMY_THREAD_MS   = 120;
constexpr int PROJ_THREAD_MS    = 60;
constexpr int HUD_THREAD_MS     = 200;

// Caracteres de referencia
constexpr char CHAR_PIT         = '@';  // referencia lógica — visualmente reemplazado por PIT_SPRITES
constexpr char CHAR_MONOEYE     = 'M';  // referencia lógica — visualmente reemplazado por ENEMY_SPRITES[0]
constexpr char CHAR_SHEMUM      = 'S';  // referencia lógica — visualmente reemplazado por ENEMY_SPRITES[1]
constexpr char CHAR_REAPER      = 'R';  // referencia lógica — visualmente reemplazado por ENEMY_SPRITES[2]
constexpr char CHAR_REAPETTE    = 'r';  // referencia lógica — visualmente reemplazado por ENEMY_SPRITES[3]
constexpr char CHAR_MEDUSA      = 'G';  // referencia lógica — visualmente reemplazado por ENEMY_SPRITES[4]
constexpr char CHAR_PROJ_PIT    = '>';  // referencia lógica — visualmente reemplazado por '>' en render()
constexpr char CHAR_PROJ_ENEMY  = 'o';  // referencia lógica — visualmente reemplazado por '*' en render()
constexpr char CHAR_PLATFORM    = '=';  // CAMBIADO: era '*' — render() usa put() con este valor, debe coincidir
constexpr char CHAR_SHOP        = '$';  // sin cambios
constexpr char CHAR_EMPTY       = ' ';  // sin cambios
constexpr char CHAR_WALL        = '#';  // sin cambios