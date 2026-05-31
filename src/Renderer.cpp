#include "Renderer.h"
#include "HUD.h"
#include <cstdio>
#include <cstring>

static inline void mv(int x, int y) { printf("\033[%d;%dH", y, x); }
static inline void cls()            { printf("\033[H\033[2J"); fflush(stdout); }

// Colores ANSI
#define COL_GOLD    "\033[33m"
#define COL_CYAN    "\033[36m"
#define COL_RED     "\033[31m"
#define COL_WHITE   "\033[97m"
#define COL_DIM     "\033[2m"
#define COL_BOLD    "\033[1m"
#define COL_RESET   "\033[0m"

// Sprite sheet de Pit  6 frames x 4 filas x 5 cols 
// El punto lógico de colisión (pos.x, pos.y) cae en col 2, fila 3 del sprite.
const char* const Renderer::PIT_SPRITES[static_cast<int>(SpriteFrame::FRAME_COUNT)][4] = {
    { " (Ö) ", "  ║  ", " )═( ", " ¯ ¯ " },  // IDLE
    { " (Ö) ", "  ║  ", " )═► ", " ¯ ¯ " },  // WALK1
    { " (Ö) ", "  ║  ", " ◄═( ", " ¯ ¯ " },  // WALK2
    { " (Ö) ", " /|\\ ", "  ║  ", "  ▲  " },  // JUMP
    { " [×] ", " (║) ", " )═( ", " ¯ ¯ " },  // DAMAGE
    { "\\(Ö)/", "  ║  ", " )═( ", " ¯ ¯ " },  // VICTORY
};

// Sprite sheet de enemigos 3 filas x 5 tipos x 5 cols
// EnemyType: MONOEYE=0 SHEMUM=1 REAPER=2 REAPETTE=3 MEDUSA=4
const char* const Renderer::ENEMY_SPRITES[5][3] = {
    { " ░◉░ ", "~~~~~", "     " },  // MONOEYE
    { "≈≈S≈≈", "/\\-\\ ", "     " },  // SHEMUM
    { " ╔R╗ ", "/║║║\\", " ▼▼▼ " },  // REAPER
    { " (r) ", " /|\\ ", "     " },  // REAPETTE
    { "~<G>~", "║|||║", "▓▓▓▓▓" },  // MEDUSA
};

Renderer::Renderer() {
    printf("\033[?25l");
    memset(front_, ' ', sizeof(front_));
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        front_[y][SCREEN_WIDTH] = '\0';
    clear();
}

void Renderer::render(const GameState& gs, const Level& level) {
    // No dibujar si no estamos en gameplay activo
    GameStatus st = gs.status.load();
    if (st != GameStatus::RUNNING && st != GameStatus::BOSS &&
        st != GameStatus::PAUSED)
        return;

    ++frame_;
    clear();

    // Detectar movimiento real de Pit para elegir IDLE vs WALK
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        pitMoving_ = (gs.pit.pos.x != prevPitX_ || gs.pit.pos.y != prevPitY_);
        prevPitX_  = gs.pit.pos.x;
        prevPitY_  = gs.pit.pos.y;
    }

    // Fondo primero 
    drawBackground(gs.phase.load(), gs.status.load());

    // Plataformas con '=' ASCII
    for (auto& p : level.getPlatforms())
        for (int i = 0; i < p.length; ++i)
            put(p.x + i, p.y + GAME_ROW_START, '=');

    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyMutex));
        for (auto& e : gs.enemies) {
            if (!e.alive) continue;
            drawEnemySprite(e.pos.x, e.pos.y + GAME_ROW_START, e.type, e.dir);
        }
    }

    // Proyectiles enemigos
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyProjMutex));
        for (int i = 0; i < MAX_ENEMY_PROJ; ++i)
            if (gs.enemyProjs[i].active) {
                put(gs.enemyProjs[i].pos.x,     gs.enemyProjs[i].pos.y + GAME_ROW_START, '*');
                put(gs.enemyProjs[i].pos.x + 1, gs.enemyProjs[i].pos.y + GAME_ROW_START, '~');
            }
    }

    // Proyectiles del jugador 
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.playerProjMutex));
        for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
            if (!gs.playerProjs[i].active) continue;
            char head, trail;
            if      (gs.playerProjs[i].dir == Direction::LEFT) { head = '<'; trail = '.'; }
            else if (gs.playerProjs[i].dir == Direction::UP)   { head = '^'; trail = '|'; }
            else                                               { head = '>'; trail = '.'; }
            put(gs.playerProjs[i].pos.x, gs.playerProjs[i].pos.y + GAME_ROW_START, head);
            int tx = gs.playerProjs[i].pos.x +
                     (gs.playerProjs[i].dir == Direction::LEFT ? 1 : -1);
            put(tx, gs.playerProjs[i].pos.y + GAME_ROW_START, trail);
        }
    }

    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        if (gs.pit.invincibleTicks == 0 || frame_ % 4 >= 2) {
            SpriteFrame f = getPlayerFrame(gs.pit);
            bool left = (gs.pit.facing == Direction::LEFT);
            drawPitSprite(gs.pit.pos.x, gs.pit.pos.y + GAME_ROW_START, f, left);
        }
    }

    drawHUD(gs);
    flush();
    memcpy(front_, back_, sizeof(back_));
    fflush(stdout);
}

// 4 fondos: fase 1 = Templo Celestial, 2 = Ruinas, 3 = Fortaleza, BOSS = Medusa
void Renderer::drawBackground(int phase, GameStatus status) {
    const int top = GAME_ROW_START;
    const int mid = GAME_ROW_START + 8;
    const int bot = SCREEN_HEIGHT - 2;

    if (status == GameStatus::BOSS) {
        putStr(0, top,   "▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓");
        putStr(0, top+1, "║~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~≈~║");
        putStr(0, mid,   "║     ║                                                                    ║    ");
        putStr(0, mid+1, "║ ▓▓▓ ║                                                                    ║ ▓▓▓");
        putStr(0, bot,   "▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼");
    } else if (phase == 1) {
        putStr(0, top,   "  ✦       ✦          ✦              ✦          ✦        ✦             ✦     ");
        putStr(0, top+1, "      ✦        ✦               ✦        ✦                   ✦              ✦ ");
        putStr(0, mid,   "  │       │           │              │          │        │             │     ");
        putStr(0, mid+1, "  │       │           │              │          │        │             │     ");
        putStr(0, bot,   "~~╨~~~~~~~╨~~~~~~~~~~~╨~~~~~~~~~~~~~~╨~~~~~~~~~~╨~~~~~~~~╨~~~~~~~~~~~~~╨~~~~~");
    } else if (phase == 2) {
        putStr(0, top,   "  _Λ_     _Λ_         _Λ_            _Λ_        _Λ_      _Λ_          _Λ_  ");
        putStr(0, top+1, " /   \\   /   \\       /   \\          /   \\      /   \\    /   \\        /   / ");
        putStr(0, mid,   "  ║  ╫    ║              ║    ╫         ║            ║  ╫              ║    ");
        putStr(0, mid+1, "  ╨  ╨    ╨              ╨    ╨         ╨            ╨  ╨              ╨    ");
        putStr(0, bot,   "▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓");
    } else {
        putStr(0, top,   "▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓");
        putStr(0, top+1, "║                                                                              ║");
        putStr(0, mid,   "║  ▐▌     ▐▌      ▐▌       ▐▌      ▐▌       ▐▌      ▐▌       ▐▌      ▐▌     ║");
        putStr(0, mid+1, "║                                                                              ║");
        putStr(0, bot,   "╚═══════════════════════════════════════════════════════════════════════════════");
    }
}


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
    for (int row = 0; row < 4; ++row) {
        int absRow = drawY + row;
        if (absRow < GAME_ROW_START) continue;
        if (absRow >= SCREEN_HEIGHT) break;
        mv(drawX + 1, absRow + 1);
        printf("%s", PIT_SPRITES[fi][row]);
    }
}

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

// pitMoving_ detecta movimiento real — pit.facing siempre es LEFT o RIGHT,
SpriteFrame Renderer::getPlayerFrame(const Player& pit) const {
    if (pit.invincibleTicks > 0 && frame_ % 6 < 3) return SpriteFrame::DAMAGE;
    if (!pit.onGround)                              return SpriteFrame::JUMP;
    if (pitMoving_)
        return (frame_ / 6) % 2 == 0 ? SpriteFrame::WALK1 : SpriteFrame::WALK2;
    return SpriteFrame::IDLE;
}

void Renderer::drawHUD(const GameState& gs) {
    HUDData h = HUD::getSnapshot();

    putStr(0, 0, "╔");
    for (int x = 1; x < SCREEN_WIDTH - 1; ++x) putStr(x, 0, "═");
    putStr(SCREEN_WIDTH - 1, 0, "╗");

    putStr(0, 1, "║");
    putStr(SCREEN_WIDTH - 1, 1, "║");

    char lives[16];
    snprintf(lives, sizeof(lives), " %s%s%s ",
        h.lives > 0 ? "\xe2\x99\xa5" : "\xe2\x99\xa1",
        h.lives > 1 ? "\xe2\x99\xa5" : "\xe2\x99\xa1",
        h.lives > 2 ? "\xe2\x99\xa5" : "\xe2\x99\xa1");

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

    char line[200];
    snprintf(line, sizeof(line),
        " LVS:%s  HP:%s  *%05d  F:%d  Enm:%02d  Hrt:%02d ",
        lives, hpBar, h.score, h.phase, alive, h.hearts);
    putStr(1, 1, line);

    putStr(0, 2, "╠");
    for (int x = 1; x < SCREEN_WIDTH - 1; ++x) putStr(x, 2, "═");
    putStr(SCREEN_WIDTH - 1, 2, "╣");

    putStr(0, 3, "║");
    putStr(SCREEN_WIDTH - 1, 3, "║");
    const char* ctrl = " [SPC]►Fire  [W]▲Jump  [A/D]Move  [P]Pause  [ESC]Menu";
    if (h.status == GameStatus::PAUSED)
        ctrl = " ⏸ PAUSED ⏸   [P] Resume                  [ESC] Main Menu";
    if (h.status == GameStatus::BOSS)
        ctrl = " ⚠ BOSS: MEDUSA!  Defeat the Gorgon and rescue Palutena!  ⚠";
    putStr(1, 3, ctrl);
}

void Renderer::renderMenu() {
    cls();
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
    printf("║  (c) 2025  Universidad del Valle de Guatemala  ·  CC3086  ·  Grupo 4       ║\n");
    printf(COL_GOLD COL_BOLD);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf(COL_RESET);
    fflush(stdout);
}

void Renderer::renderInstructions() {
    cls();
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


void Renderer::renderPause(const GameState& gs) {
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

void Renderer::clear() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        memset(back_[y], ' ', SCREEN_WIDTH);
        back_[y][SCREEN_WIDTH] = '\0';
    }
}

void Renderer::put(int x, int y, char c) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        back_[y][x] = c;
}

void Renderer::putStr(int x, int y, const char* s) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    for (int i = 0; s[i] && x + i < SCREEN_WIDTH; ++i)
        if (x + i >= 0)
            back_[y][x + i] = s[i];
}

void Renderer::flush() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            if (back_[y][x] != front_[y][x]) {
                mv(x + 1, y + 1);
                putchar(back_[y][x]);
            }
}