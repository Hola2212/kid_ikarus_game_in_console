// Renderer.cpp — Capa visual unificada (Fase 3 · CC3086 · UVG 2025)
// Sistema: mv()+printf() directo en toda la capa visual.
// Sin buffer back_[]/front_[]. Sin put()/putStr()/flush().
// UTF-8 funciona porque los strings llegan al terminal completos.
#include "Renderer.h"
#include "HUD.h"
#include <cstdio>
#include <cstring>

static inline void mv(int x, int y) { printf("\033[%d;%dH", y, x); }
static inline void cls()            { printf("\033[H\033[2J"); fflush(stdout); }

// Colores ANSI — solo pantallas estáticas, nunca en render() principal
#define COL_GOLD    "\033[33m"
#define COL_CYAN    "\033[36m"
#define COL_RED     "\033[31m"
#define COL_WHITE   "\033[97m"
#define COL_DIM     "\033[2m"
#define COL_BOLD    "\033[1m"
#define COL_RESET   "\033[0m"

// =============================================================================
// SPRITE SHEETS
// mv()+printf() directo — el string UTF-8 llega al terminal intacto
// =============================================================================
const char* const Renderer::PIT_SPRITES[static_cast<int>(SpriteFrame::FRAME_COUNT)][4] = {
    { " (Ö) ", "  ║  ", " )═( ", " ¯ ¯ " },  // IDLE
    { " (Ö) ", "  ║  ", " )═► ", " ¯ ¯ " },  // WALK1
    { " (Ö) ", "  ║  ", " ◄═( ", " ¯ ¯ " },  // WALK2
    { " (Ö) ", " /|\\ ", "  ║  ", "  ▲  " },  // JUMP
    { " [×] ", " (║) ", " )═( ", " ¯ ¯ " },  // DAMAGE
    { "\\(Ö)/", "  ║  ", " )═( ", " ¯ ¯ " },  // VICTORY
};

const char* const Renderer::ENEMY_SPRITES[5][3] = {
    { " ░◉░ ", "~~~~~", "     " },  // MONOEYE
    { "≈≈S≈≈", "/\\-\\ ", "     " },  // SHEMUM
    { " ╔R╗ ", "/║║║\\", " ▼▼▼ " },  // REAPER
    { " (r) ", " /|\\ ", "     " },  // REAPETTE
    { "~<G>~", "║|||║", "▓▓▓▓▓" },  // MEDUSA
};

// =============================================================================
// Constructor
// =============================================================================
Renderer::Renderer() {
    printf("\033[?25l");  // ocultar cursor
    // Inicializar tracking de posiciones anteriores
    prevDrawPitX_ = -99;
    prevDrawPitY_ = -99;
    for (int i = 0; i < MAX_TRACKED_ENEMIES; ++i) {
        prevEnemyX_[i] = -99;
        prevEnemyY_[i] = -99;
        prevEnemyActive_[i] = false;
    }
    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
        prevProjX_[i] = -99;
        prevProjY_[i] = -99;
        prevProjActive_[i] = false;
    }
    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
        prevEProjX_[i] = -99;
        prevEProjY_[i] = -99;
        prevEProjActive_[i] = false;
    }
    fflush(stdout);
}

// =============================================================================
// clearSprite — borra un área rectangular con espacios
// Guard: no tocar filas del HUD (1-4 del terminal = rows 1-4)
// =============================================================================
void Renderer::clearSprite(int drawX, int drawY, int cols, int rows) {
    for (int row = 0; row < rows; ++row) {
        int absRow = drawY + row;
        if (absRow + 1 < GAME_ROW_START + 1) continue;  // no pisar HUD
        if (absRow >= SCREEN_HEIGHT) break;
        mv(drawX + 1, absRow + 1);
        for (int c = 0; c < cols; ++c) printf(" ");
    }
}

// =============================================================================
// RENDER PRINCIPAL
// fullRedraw_: cls() + fondo + plataformas completos
// Frame normal: solo borrar/redibujar elementos que se mueven
// =============================================================================
void Renderer::render(const GameState& gs, const Level& level) {
    GameStatus st = gs.status.load();
    if (st != GameStatus::RUNNING && st != GameStatus::BOSS &&
        st != GameStatus::PAUSED)
        return;

    ++frame_;

    // Limpiar overlay de pausa al reanudar
    if (prevStatus_ == GameStatus::PAUSED && st != GameStatus::PAUSED)
        fullRedraw_ = true;
    prevStatus_ = st;

    // fullRedraw_: cls() + fondo + reset tracking
    if (fullRedraw_) {
        cls();
        drawBackground(gs.phase.load(), st);
        // Resetear tracking para que todo se redibuje
        prevDrawPitX_ = -99;
        for (int i = 0; i < MAX_TRACKED_ENEMIES; ++i) prevEnemyActive_[i] = false;
        for (int i = 0; i < MAX_PLAYER_PROJ;     ++i) prevProjActive_[i]  = false;
        for (int i = 0; i < MAX_ENEMY_PROJ;      ++i) prevEProjActive_[i] = false;
        fullRedraw_ = false;
    }

    // ── Detectar movimiento de Pit ────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        pitMoving_ = (gs.pit.pos.x != prevPitX_ || gs.pit.pos.y != prevPitY_);
        prevPitX_  = gs.pit.pos.x;
        prevPitY_  = gs.pit.pos.y;
    }

    // ── Proyectiles enemigos — borrar anterior, dibujar nuevo ────────────────
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyProjMutex));
        for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
            // Borrar posición anterior si cambió o se desactivó
            if (prevEProjActive_[i]) {
                int py = prevEProjY_[i] + GAME_ROW_START;
                if (py >= GAME_ROW_START && py < SCREEN_HEIGHT) {
                    mv(prevEProjX_[i] + 1, py + 1); printf("  ");
                }
            }
            prevEProjActive_[i] = gs.enemyProjs[i].active;
            if (gs.enemyProjs[i].active) {
                prevEProjX_[i] = gs.enemyProjs[i].pos.x;
                prevEProjY_[i] = gs.enemyProjs[i].pos.y;
                int py = gs.enemyProjs[i].pos.y + GAME_ROW_START;
                if (py >= GAME_ROW_START && py < SCREEN_HEIGHT) {
                    mv(gs.enemyProjs[i].pos.x + 1, py + 1);
                    printf("*~");
                }
            }
        }
    }

    // ── Proyectiles del jugador — borrar anterior, dibujar nuevo ─────────────
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.playerProjMutex));
        for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
            if (prevProjActive_[i]) {
                int py = prevProjY_[i] + GAME_ROW_START;
                if (py >= GAME_ROW_START && py < SCREEN_HEIGHT) {
                    mv(prevProjX_[i] + 1, py + 1); printf("  ");
                }
            }
            prevProjActive_[i] = gs.playerProjs[i].active;
            if (gs.playerProjs[i].active) {
                prevProjX_[i] = gs.playerProjs[i].pos.x;
                prevProjY_[i] = gs.playerProjs[i].pos.y;
                int py = gs.playerProjs[i].pos.y + GAME_ROW_START;
                if (py >= GAME_ROW_START && py < SCREEN_HEIGHT) {
                    char head;
                    if      (gs.playerProjs[i].dir == Direction::LEFT) head = '<';
                    else if (gs.playerProjs[i].dir == Direction::UP)   head = '^';
                    else                                               head = '>';
                    mv(gs.playerProjs[i].pos.x + 1, py + 1);
                    printf("%c.", head);
                }
            }
        }
    }

    // ── Enemigos ──────────────────────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyMutex));
        int idx = 0;
        for (auto& e : gs.enemies) {
            if (idx >= MAX_TRACKED_ENEMIES) break;
            // Borrar sprite anterior si el enemigo se movió
            if (prevEnemyActive_[idx]) {
                clearSprite(prevEnemyX_[idx] - 2, prevEnemyY_[idx] - 2, 5, 3);
            }
            prevEnemyActive_[idx] = e.alive;
            if (e.alive) {
                prevEnemyX_[idx] = e.pos.x;
                prevEnemyY_[idx] = e.pos.y + GAME_ROW_START;
                drawEnemySprite(e.pos.x, e.pos.y + GAME_ROW_START, e.type, e.dir);
            }
            ++idx;
        }
        // Borrar enemigos que ya no existen
        for (; idx < MAX_TRACKED_ENEMIES; ++idx) {
            if (prevEnemyActive_[idx]) {
                clearSprite(prevEnemyX_[idx] - 2, prevEnemyY_[idx] - 2, 5, 3);
                prevEnemyActive_[idx] = false;
            }
        }
    }

    // Plataformas — después de todos los clearSprite(), antes de dibujar sprites
    // Así se restauran los '=' borrados por clearSprite() en este mismo frame
    for (auto& p : level.getPlatforms()) {
        int drawY = p.y + GAME_ROW_START;
        if (drawY < GAME_ROW_START || drawY >= SCREEN_HEIGHT) continue;
        int x0 = p.x;
        int x1 = std::min(p.x + p.length, SCREEN_WIDTH);
        if (x0 >= x1) continue;
        mv(x0 + 1, drawY + 1);
        for (int i = x0; i < x1; ++i) printf("=");
    }

    // ── Pit ───────────────────────────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        if (gs.pit.invincibleTicks == 0 || frame_ % 4 >= 2) {
            SpriteFrame f = getPlayerFrame(gs.pit);
            bool left = (gs.pit.facing == Direction::LEFT);
            drawPitSprite(gs.pit.pos.x, gs.pit.pos.y + GAME_ROW_START, f, left);
        }
    }

    drawHUD(gs);
    fflush(stdout);
}

// =============================================================================
// drawBackground — fondo temático por fase, mv()+printf() directo
// Coordenadas: top=fila 4, mid=fila 12, bot=fila 22
// =============================================================================
void Renderer::drawBackground(int phase, GameStatus status) {
    const int top = GAME_ROW_START;
    const int mid = GAME_ROW_START + 8;
    const int bot = SCREEN_HEIGHT - 2;

    if (status == GameStatus::BOSS) {
        mv(1, top+1);   printf("▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓");
        mv(1, top+2);   printf("║~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~║");
        mv(1, mid+1);   printf("║     ║                                                                    ║");
        mv(1, mid+2);   printf("║ ▓▓▓ ║                                                                    ║");
        mv(1, bot+1);   printf("▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼");
    } else if (phase == 1) {
        mv(1, top+1);   printf("  ✦       ✦          ✦              ✦          ✦        ✦             ✦    ");
        mv(1, top+2);   printf("      ✦        ✦               ✦        ✦                   ✦             ");
        mv(1, mid+1);   printf("  │       │           │              │          │        │             │   ");
        mv(1, mid+2);   printf("  │       │           │              │          │        │             │   ");
        mv(1, bot+1);   printf("~~╨~~~~~~~╨~~~~~~~~~~~╨~~~~~~~~~~~~~~╨~~~~~~~~~~╨~~~~~~~~╨~~~~~~~~~~~~~╨~~~");
    } else if (phase == 2) {
        mv(1, top+1);   printf("  _Λ_     _Λ_         _Λ_            _Λ_        _Λ_      _Λ_          _Λ_ ");
        mv(1, top+2);   printf(" /   \\   /   \\       /   \\          /   \\      /   \\    /   \\        /   /");
        mv(1, mid+1);   printf("  ║  ╫    ║              ║    ╫         ║            ║  ╫              ║  ");
        mv(1, mid+2);   printf("  ╨  ╨    ╨              ╨    ╨         ╨            ╨  ╨              ╨  ");
        mv(1, bot+1);   printf("▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓");
    } else {
        mv(1, top+1);   printf("▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓");
        mv(1, top+2);   printf("║                                                                          ║");
        mv(1, mid+1);   printf("║  ▐▌     ▐▌      ▐▌       ▐▌      ▐▌       ▐▌      ▐▌       ▐▌      ▐▌ ║");
        mv(1, mid+2);   printf("║                                                                          ║");
        mv(1, bot+1);   printf("╚══════════════════════════════════════════════════════════════════════════");
    }
}

// =============================================================================
// drawPitSprite — borra sprite anterior, dibuja nuevo
// Guard: absRow >= GAME_ROW_START (no pisar HUD)
// =============================================================================
void Renderer::drawPitSprite(int x, int y, SpriteFrame f, bool facingLeft) {
    int fi = static_cast<int>(f);
    if (facingLeft) {
        if (f == SpriteFrame::WALK1) fi = static_cast<int>(SpriteFrame::WALK2);
        else if (f == SpriteFrame::WALK2) fi = static_cast<int>(SpriteFrame::WALK1);
    }
    int drawX = x - 2;
    int drawY = y - 3;
    if (drawX + 5 > SCREEN_WIDTH) drawX = SCREEN_WIDTH - 5;
    if (drawX < 0)                drawX = 0;

    // Borrar sprite anterior
    if (prevDrawPitX_ > -99)
        clearSprite(prevDrawPitX_, prevDrawPitY_, 5, 4);

    prevDrawPitX_ = drawX;
    prevDrawPitY_ = drawY;

    // Dibujar sprite nuevo
    for (int row = 0; row < 4; ++row) {
        int absRow = drawY + row;
        if (absRow < GAME_ROW_START) continue;
        if (absRow >= SCREEN_HEIGHT) break;
        mv(drawX + 1, absRow + 1);
        printf("%s", PIT_SPRITES[fi][row]);
    }
}

// =============================================================================
// drawEnemySprite — mv()+printf() directo
// =============================================================================
void Renderer::drawEnemySprite(int x, int y, EnemyType type, Direction dir) {
    (void)dir;
    int ti = static_cast<int>(type);
    int drawX = x - 2;
    if (drawX + 5 > SCREEN_WIDTH) drawX = SCREEN_WIDTH - 5;
    if (drawX < 0)                drawX = 0;
    for (int row = 0; row < 3; ++row) {
        int drawY = y - 2 + row;
        if (drawY < GAME_ROW_START) continue;
        if (drawY >= SCREEN_HEIGHT) break;
        mv(drawX + 1, drawY + 1);
        printf("%s", ENEMY_SPRITES[ti][row]);
    }
}

// =============================================================================
// getPlayerFrame — pitMoving_ detecta IDLE real
// pit.facing siempre es LEFT o RIGHT, nunca NONE
// =============================================================================
SpriteFrame Renderer::getPlayerFrame(const Player& pit) const {
    if (pit.invincibleTicks > 0 && frame_ % 6 < 3) return SpriteFrame::DAMAGE;
    if (!pit.onGround)                              return SpriteFrame::JUMP;
    if (pitMoving_)
        return (frame_ / 6) % 2 == 0 ? SpriteFrame::WALK1 : SpriteFrame::WALK2;
    return SpriteFrame::IDLE;
}

// =============================================================================
// drawHUD — mv()+printf() directo, filas 1-4 del terminal
// UTF-8 funciona porque printf envía el string completo
// buffer 200 bytes en snprintf porque ♥ ocupa 3 bytes UTF-8
// =============================================================================
void Renderer::drawHUD(const GameState& gs) {
    HUDData h = HUD::getSnapshot();

    mv(1, 1); printf("╔");
    for (int x = 1; x < SCREEN_WIDTH - 1; ++x) printf("═");
    printf("╗");

    char lives[16];
    snprintf(lives, sizeof(lives), " %s%s%s ",
        h.lives > 0 ? "♥" : "♡",
        h.lives > 1 ? "♥" : "♡",
        h.lives > 2 ? "♥" : "♡");

    char hpBar[32];
    hpBar[0] = '[';
    for (int i = 0; i < MAX_HP; ++i)
        hpBar[i + 1] = (i < h.hp) ? '#' : '-';
    hpBar[MAX_HP + 1] = ']';
    hpBar[MAX_HP + 2] = '\0';

    int alive = 0;
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyMutex));
        for (auto& e : gs.enemies) if (e.alive) ++alive;
    }

    mv(1, 2);
    printf("║ LVS:%s HP:%s ★%05d  F:%d  Enm:%02d  Hrt:%02d",
           lives, hpBar, h.score, h.phase, alive, h.hearts);
    mv(SCREEN_WIDTH, 2); printf("║");

    mv(1, 3); printf("╠");
    for (int x = 1; x < SCREEN_WIDTH - 1; ++x) printf("═");
    printf("╣");

    mv(1, 4); printf("║");
    const char* ctrl = " [SPC]►Fire  [W]▲Jump  [A/D]Move  [P]Pause  [ESC]Menu";
    if (h.status == GameStatus::PAUSED)
        ctrl = " ⏸ PAUSED ⏸   [P] Resume                  [ESC] Main Menu";
    if (h.status == GameStatus::BOSS)
        ctrl = " ⚠ BOSS: MEDUSA!  Defeat the Gorgon and rescue Palutena!  ⚠";
    printf("%s", ctrl);
    mv(SCREEN_WIDTH, 4); printf("║");

    fflush(stdout);
}

// =============================================================================
// PANTALLAS ESTÁTICAS — cls() + limpiar tracking + fullRedraw_
// =============================================================================
void Renderer::renderMenu() {
    cls();
    // Resetear tracking para que render() no deje residuos al volver al juego
    prevDrawPitX_ = -99;
    for (int i = 0; i < MAX_TRACKED_ENEMIES; ++i) prevEnemyActive_[i] = false;
    for (int i = 0; i < MAX_PLAYER_PROJ;     ++i) prevProjActive_[i]  = false;
    for (int i = 0; i < MAX_ENEMY_PROJ;      ++i) prevEProjActive_[i] = false;
    fullRedraw_ = true;
    printf(COL_GOLD COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦  ║\n");
    printf("║                                                                              ║\n");
    printf(COL_WHITE);
    printf("║  ██╗  ██╗██╗██████╗     ██╗ ██████╗ █████╗ ██████╗ ██╗   ██╗███████╗       ║\n");
    printf("║  ██║ ██╔╝██║██╔══██╗    ██║██╔════╝██╔══██╗██╔══██╗██║   ██║██╔════╝       ║\n");
    printf("║  █████╔╝ ██║██║  ██║    ██║██║     ███████║██████╔╝██║   ██║███████╗       ║\n");
    printf("║  ██╔═██╗ ██║██║  ██║    ██║██║     ██╔══██║██╔══██╗██║   ██║╚════██║       ║\n");
    printf("║  ██║  ██╗██║██████╔╝    ██║╚██████╗██║  ██║██║  ██║╚██████╔╝███████║       ║\n");
    printf("║  ╚═╝  ╚═╝╚═╝╚═════╝     ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝       ║\n");
    printf(COL_GOLD);
    printf("║                                                                              ║\n");
    printf("║          ─── The Legend of Pit · Servant of Palutena ───                   ║\n");
    printf("║                                                                              ║\n");
    printf("║  ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦  ║\n");
    printf("║                                                                              ║\n");
    printf(COL_WHITE COL_BOLD);
    printf("║                        ►  START GAME       [1]                              ║\n");
    printf(COL_RESET COL_GOLD);
    printf("║                           CPU MODE         [2]                              ║\n");
    printf("║                           INSTRUCTIONS     [I]                              ║\n");
    printf("║                           HIGH SCORES      [S]                              ║\n");
    printf("║                           EXIT             [ESC]                            ║\n");
    printf("║                                                                              ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf(COL_DIM COL_GOLD);
    printf("║   Universidad del Valle de Guatemala  ·  CC3086  ·  Grupo 4       ║\n");
    printf(COL_GOLD COL_BOLD);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderInstructions() {
    cls();
    fullRedraw_ = true;
    printf(COL_CYAN COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║           ─── MANUAL DEL HÉROE ───   Kid Icarus · Servant of Palutena     ║\n");
    printf("╠═══════════════════════╦════════════════════════════════════════════════════╣\n");
    printf("║  CONTROLES            ║  ELEMENTOS DEL MUNDO                               ║\n");
    printf("║                       ║                                                    ║\n");
    printf("║  [A] / [D]   ◄  ►    ║  ═══  Plataforma            (Ö)  = Pit (tú)       ║\n");
    printf("║  [W]         ▲ Saltar ║   ░◉░  Monoeye    +30 pts    ║                    ║\n");
    printf("║  [S]         ▼ Agachar║  ≈≈S≈  Shemum     +20 pts   )═(                  ║\n");
    printf("║  [SPACE]     ► Disp.  ║   ╔R╗  Reaper     +50 pts   ¯ ¯                  ║\n");
    printf("║  [C]         ▲ Disp.  ║   (r)  Reapette   +10 pts                         ║\n");
    printf("║  [P]         Pausa    ║  ~<G>~ MEDUSA!    JEFE                            ║\n");
    printf("║  [ESC]       Menú     ║                                                    ║\n");
    printf("║                       ║  ♥♥♥ = Vidas   [####--] = HP   ♦ = Corazones     ║\n");
    printf("╠═══════════════════════╩════════════════════════════════════════════════════╣\n");
    printf("║  OBJETIVO: Asciende 3 fases, derrota a Medusa, rescata a Palutena         ║\n");
    printf("║                                                                            ║\n");
    printf("║   Fase 1: Templo Celestial    Fase 2: Ruinas Sagradas                     ║\n");
    printf("║   Fase 3: Fortaleza Oscura    Jefe:   Templo de Medusa                    ║\n");
    printf("║                                                                            ║\n");
    printf("║                       [ Any key ] Volver al Olimpo                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderScores() {
    cls();
    fullRedraw_ = true;
    printf(COL_GOLD COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║   ✦ ══════════════  SALÓN DE HÉROES DE PALUTENA  ══════════════ ✦          ║\n");
    printf("╠════╦════════════════════╦════════════╦══════════╦═══════════════════════════╣\n");
    printf("║  # ║  NOMBRE            ║   SCORE    ║  FASE    ║  RANGO                   ║\n");
    printf("╠════╬════════════════════╬════════════╬══════════╬═══════════════════════════╣\n");
    printf(COL_RESET COL_GOLD);
    const char* ranks[] = {
        "  ★ Campeón Divino    ",
        "  ✦ Héroe Legendario  ",
        "  ◆ Guerrero Sagrado  ",
        "  ◇ Guardián del Cielo",
        "  · Aprendiz Alado    ",
    };
    FILE* f = fopen("scores.txt", "r");
    char name[32]; int score, phase, rank = 0;
    if (f) {
        while (rank < MAX_SCORES && fscanf(f, "%31s %d %d", name, &score, &phase) == 3) {
            if (rank == 0) printf(COL_WHITE COL_BOLD);
            else           printf(COL_GOLD COL_RESET COL_GOLD);
            printf("║  %d ║  %-18s  ║  %08d  ║  Fase %-2d  ║ %s ║\n",
                   rank + 1, name, score, phase, ranks[rank]);
            ++rank;
        }
        fclose(f);
    }
    while (rank < MAX_SCORES) {
        printf(COL_DIM "║  %d ║  ·····             ║  ·······   ║  ······   ║ ······················  ║\n" COL_RESET COL_GOLD, rank + 1);
        ++rank;
    }
    printf("╠════╩════════════════════╩════════════╩══════════╩═══════════════════════════╣\n");
    printf("║                     [ Any key ]  Volver al Olimpo                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderShop(const GameState& gs) {
    cls();
    fullRedraw_ = true;
    int hearts, hp;
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        hearts = gs.pit.hearts;
        hp     = gs.pit.hp;
    }
    printf(COL_CYAN COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║               ─── SANTUARIO DE PALUTENA ───                                ║\n");
    printf("║              « Restore thy strength, brave hero »                          ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║                                                                              ║\n");
    printf("║   Corazones actuales:  ♦ %-4d       HP actual: %2d / %2d                    ║\n",
           hearts, hp, MAX_HP);
    printf("║                                                                              ║\n");
    printf("║   HP  [");
    for (int i = 0; i < MAX_HP; ++i) printf("%c", i < hp ? '#' : '-');
    printf("]                                               ║\n");
    printf("║                                                                              ║\n");
    printf("║   [C]  Comprar +%d HP   (cuesta %d corazones)                               ║\n",
           SHOP_HP_GAIN, SHOP_HEART_COST);
    printf("║   [Q]  Continuar a Fase 3  ─── El jefe final aguarda...                    ║\n");
    printf("║                                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

// No llama cls() — campo de juego visible detrás del overlay
// Actualiza prevStatus_ para que render() detecte la transición PAUSED→RUNNING
void Renderer::renderPause(const GameState& gs) {
    prevStatus_ = GameStatus::PAUSED;
    HUDData h = HUD::getSnapshot();
    mv(10, 7);
    printf(COL_CYAN COL_BOLD "╔══════════════════════════════════════════════════╗" COL_RESET);
    mv(10, 8);
    printf(COL_CYAN COL_BOLD "║    ⏸  JUEGO EN PAUSA  ⏸                        ║" COL_RESET);
    mv(10, 9);
    printf(COL_CYAN "╠══════════════════════════════════════════════════╣" COL_RESET);
    mv(10,10);
    printf(COL_CYAN "║  Vidas  %c%c%c     Score  %05d     Fase  %d        ║" COL_RESET,
           h.lives>0?'*':'.', h.lives>1?'*':'.', h.lives>2?'*':'.',
           h.score, h.phase);
    mv(10,11);
    printf(COL_CYAN "║  HP  [");
    for (int i = 0; i < MAX_HP; ++i) printf("%c", i < h.hp ? '#' : '-');
    printf("]   ♦ %02d corazones           ║" COL_RESET, h.hearts);
    mv(10,12);
    printf(COL_CYAN "╠══════════════════════════════════════════════════╣" COL_RESET);
    mv(10,13);
    printf(COL_CYAN "║      [ P ] Reanudar       [ ESC ] Menú           ║" COL_RESET);
    mv(10,14);
    printf(COL_CYAN COL_BOLD "╚══════════════════════════════════════════════════╝" COL_RESET);
    fflush(stdout);
}

void Renderer::renderGameOver() {
    cls();
    fullRedraw_ = true;
    HUDData h = HUD::getSnapshot();
    printf(COL_RED COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                              ║\n");
    printf("║                        · · · · · · · · · · ·                                ║\n");
    printf("║                                                                              ║\n");
    printf("║  ██████╗  █████╗ ███╗   ███╗███████╗    ██████╗ ██╗   ██╗███████╗██████╗   ║\n");
    printf("║ ██╔════╝ ██╔══██╗████╗ ████║██╔════╝   ██╔═══██╗██║   ██║██╔════╝██╔══██╗  ║\n");
    printf("║ ██║  ███╗███████║██╔████╔██║█████╗     ██║   ██║██║   ██║█████╗  ██████╔╝  ║\n");
    printf("║ ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝     ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗  ║\n");
    printf("║ ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗   ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║  ║\n");
    printf("║  ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝    ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝  ║\n");
    printf("║                                                                              ║\n");
    printf(COL_RESET COL_RED);
    printf("║         ─── Pit ha caído en las sombras del Inframundo ───                  ║\n");
    printf("║                                                                              ║\n");
    printf("║                       [×]                                                   ║\n");
    printf("║                       (║)     ← El héroe ha caído                           ║\n");
    printf("║                     ~~~║~~~                                                 ║\n");
    printf("║                                                                              ║\n");
    printf(COL_GOLD COL_BOLD);
    printf("║          Score final: %05d           Fase alcanzada: %d                    ║\n",
           h.score, h.phase);
    printf("║                                                                              ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf(COL_RESET COL_RED);
    printf("║              [ R ] Jugar de nuevo          [ M ] Menú                      ║\n");
    printf(COL_RED COL_BOLD);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderVictory() {
    cls();
    fullRedraw_ = true;
    HUDData h = HUD::getSnapshot();
    printf(COL_GOLD COL_BOLD);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦  ║\n");
    printf("║                                                                              ║\n");
    printf(COL_WHITE);
    printf("║  ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗ █████╗                  ║\n");
    printf("║  ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██║██╔══██╗                 ║\n");
    printf("║  ██║   ██║██║██║        ██║   ██║   ██║██████╔╝██║███████║                 ║\n");
    printf("║  ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗██║██╔══██║                 ║\n");
    printf("║   ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║██║██║  ██║                 ║\n");
    printf("║    ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝                 ║\n");
    printf(COL_GOLD);
    printf("║                                                                              ║\n");
    printf("║        ─── Medusa derrotada. Palutena libre. El Olimpo se regocija. ───     ║\n");
    printf("║                                                                              ║\n");
    printf(COL_WHITE);
    printf("║              \\(Ö)/         \\(Ö)/         \\(Ö)/                             ║\n");
    printf("║                ║             ║             ║                                ║\n");
    printf("║               )═(           )═(           )═(                              ║\n");
    printf("║               ¯ ¯           ¯ ¯           ¯ ¯                              ║\n");
    printf(COL_GOLD);
    printf("║                                                                              ║\n");
    printf("║  ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦ ✦  ║\n");
    printf("║                                                                              ║\n");
    printf(COL_WHITE COL_BOLD);
    printf("║  ★ Score: %05d   ·   Corazones: %02d   ·   Fase: %d   ·   ¡Gloria Eterna! ★ ║\n",
           h.score, h.hearts, h.phase);
    printf(COL_GOLD);
    printf("║                                                                              ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf(COL_RESET COL_GOLD);
    printf("║              [ R ] Jugar de nuevo          [ M ] Menú                      ║\n");
    printf(COL_GOLD COL_BOLD);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderSplash(){
    cls();
    fullRedraw_ = true;
    HUDData h = HUD::getSnapshot();
    printf(COL_CYAN COL_BOLD);
    printf(R"(

    ██╗  ██╗██╗██████╗     ██╗ ██████╗ █████╗ ██████╗ ██╗   ██╗███████╗
    ██║ ██╔╝██║██╔══██╗    ██║██╔════╝██╔══██╗██╔══██╗██║   ██║██╔════╝
    █████╔╝ ██║██║  ██║    ██║██║     ███████║██████╔╝██║   ██║███████╗
    ██╔═██╗ ██║██║  ██║    ██║██║     ██╔══██║██╔══██╗██║   ██║╚════██║
    ██║  ██╗██║██████╔╝    ██║╚██████╗██║  ██║██║  ██║╚██████╔╝███████║
    ╚═╝  ╚═╝╚═╝╚═════╝     ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝

    )");
    printf(COL_RESET);
    fflush(stdout);
}