#include "Renderer.h"
#include "HUD.h"
#include <cstdio>
#include <cstring>

//helpers de terminal
static void mv(int x, int y) { printf("\033[%d;%dH", y, x); }
static void cls()             { printf("\033[H\033[J"); fflush(stdout); }


Renderer::Renderer() {
    printf("\033[?25l");   // ocultar cursor
    memset(front_, 0, sizeof(front_));
    clear();
}

// render principal
void Renderer::render(const GameState& gs, const Level& level) {
    ++frame_;
    clear();

    // plataformas
    for (auto& p : level.getPlatforms())
        for (int i = 0; i < p.length; ++i)
            put(p.x + i, p.y + GAME_ROW_START, CHAR_PLATFORM);

    // enemigos
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyMutex));
        for (auto& e : gs.enemies) {
            if (!e.alive) continue;
            char c[] = {'?',CHAR_MONOEYE,CHAR_SHEMUM,CHAR_REAPER,CHAR_REAPETTE,CHAR_MEDUSA};
            put(e.pos.x, e.pos.y + GAME_ROW_START, c[(int)e.type + 1]);
        }
    }

    // proyectiles enemigos
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.enemyProjMutex));
        for (int i = 0; i < MAX_ENEMY_PROJ; ++i)
            if (gs.enemyProjs[i].active)
                put(gs.enemyProjs[i].pos.x, gs.enemyProjs[i].pos.y + GAME_ROW_START, CHAR_PROJ_ENEMY);
    }

    // proyectiles del jugador
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.playerProjMutex));
        for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
            if (!gs.playerProjs[i].active) continue;
            char c = (gs.playerProjs[i].dir == Direction::LEFT) ? '<' :
                     (gs.playerProjs[i].dir == Direction::UP)   ? '^' : CHAR_PROJ_PIT;
            put(gs.playerProjs[i].pos.x, gs.playerProjs[i].pos.y + GAME_ROW_START, c);
        }
    }

    // jugador (parpadea si es invencible)
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        if (gs.pit.invincibleTicks == 0 || frame_ % 4 >= 2)
            put(gs.pit.pos.x, gs.pit.pos.y + GAME_ROW_START, CHAR_PIT);
    }

    drawHUD(gs);
    flush();
    memcpy(front_, back_, sizeof(back_));
    fflush(stdout);
}

//  HUD (filas 0-2)
void Renderer::drawHUD(const GameState& gs) {
    HUDData h = HUD::getSnapshot();

    // fila 0: vidas, HP, corazones, fase, score
    char row[SCREEN_WIDTH + 1];
    snprintf(row, sizeof(row),
        " V:%c%c%c  HP:[%.*s%.*s]  v:%02d  F:%d  SCORE:%05d",
        h.lives>0?'*':'-', h.lives>1?'*':'-', h.lives>2?'*':'-',
        h.hp, "==========", MAX_HP - h.hp, "          ",
        h.hearts, h.phase, h.score);
    putStr(0, 0, row);

    // fila 1: separador
    for (int x = 0; x < SCREEN_WIDTH; ++x) put(x, 1, '-');

    // fila 2: ayuda contextual
    const char* msg = " [ESPACIO]Disparar [W]Saltar [A/D]Mover [P]Pausa [ESC]Menu";
    if (h.status == GameStatus::PAUSED) msg = " *** PAUSADO *** [P] Reanudar  [ESC] Menu";
    if (h.status == GameStatus::BOSS)   msg = " *** MEDUSA! Derrota al jefe final ***";
    putStr(0, 2, msg);
}

// pantallas especiales 
void Renderer::renderMenu() {
    cls();
    printf("  +============================================================+\n");
    printf("  |         K I D   I C A R U S   ASCII                       |\n");
    printf("  |                                                            |\n");
    printf("  |   [1] Iniciar partida                                      |\n");
    printf("  |   [2] Modo CPU                                             |\n");
    printf("  |   [I] Instrucciones                                        |\n");
    printf("  |   [S] Puntajes                                             |\n");
    printf("  |   [ESC] Salir                                              |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

void Renderer::renderInstructions() {
    cls();
    printf("  +============================================================+\n");
    printf("  |              I N S T R U C C I O N E S                    |\n");
    printf("  +============================================================+\n");
    printf("  |  A/D=mover  W=saltar  S=agachar  ESPACIO=disparar         |\n");
    printf("  |  C=disparar arriba    P=pausa     ESC=menu                 |\n");
    printf("  |                                                            |\n");
    printf("  |  M=Monoeye  S=Shemum  R=Reaper  r=Reapette  G=Medusa      |\n");
    printf("  |  @=Pit      >=flecha  o=disparo enemigo  *=plataforma      |\n");
    printf("  |                                                            |\n");
    printf("  |  HUD: V:***  HP:[========  ]  v:03  F:2  SCORE:01250      |\n");
    printf("  |                                                            |\n");
    printf("  |              [Cualquier tecla] Volver                      |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

void Renderer::renderScores() {
    cls();
    printf("  +============================================================+\n");
    printf("  |           P U N T A J E S  D E S T A C A D O S            |\n");
    printf("  +============================================================+\n");
    FILE* f = fopen("scores.txt", "r");
    if (f) {
        char name[32]; int score, phase, rank = 1;
        while (rank <= MAX_SCORES && fscanf(f, "%31s %d %d", name, &score, &phase) == 3)
            printf("  |  %d. %-15s  %7d pts  Fase %d                    |\n",
                   rank++, name, score, phase);
        fclose(f);
    } else {
        printf("  |         (No hay puntajes guardados todavia)             |\n");
    }
    printf("  |                                                            |\n");
    printf("  |              [Cualquier tecla] Volver                      |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

void Renderer::renderShop(const GameState& gs) {
    cls();
    int h, hp;
    {
        std::lock_guard<std::mutex> sl(const_cast<std::mutex&>(gs.pitMutex));
        h  = gs.pit.hearts;
        hp = gs.pit.hp;
    }
    printf("  +============================================================+\n");
    printf("  |                    T I E N D A                            |\n");
    printf("  |  Corazones: %3d    HP actual: %2d/%2d                      |\n", h, hp, MAX_HP);
    printf("  |  [C] Comprar +%d HP  (cuesta %d corazones)               |\n", SHOP_HP_GAIN, SHOP_HEART_COST);
    printf("  |  [Q] Continuar a Fase 3                                   |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

void Renderer::renderPause(const GameState& gs) {
    HUDData h = HUD::getSnapshot();
    mv(1, 1);
    printf("*** PAUSADO ***  Vidas:%d  HP:%d/%d  Fase:%d  Score:%d  [P]Reanudar [ESC]Menu\n",
           h.lives, h.hp, MAX_HP, h.phase, h.score);
    fflush(stdout);
}

void Renderer::renderVictory() {
    cls();
    printf("  +============================================================+\n");
    printf("  |            V I C T O R I A !                              |\n");
    printf("  |   Medusa ha sido derrotada. Palutena esta a salvo!         |\n");
    printf("  |   [R] Jugar de nuevo    [M] Menu                          |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

void Renderer::renderGameOver() {
    cls();
    printf("  +============================================================+\n");
    printf("  |            G A M E   O V E R                              |\n");
    printf("  |   Pit ha caido en el Inframundo...                         |\n");
    printf("  |   [R] Jugar de nuevo    [M] Menu                          |\n");
    printf("  +============================================================+\n");
    fflush(stdout);
}

// buffer interno para renderizado (doble buffer)
void Renderer::clear() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        memset(back_[y], CHAR_EMPTY, SCREEN_WIDTH);
        back_[y][SCREEN_WIDTH] = '\0';
    }
}

void Renderer::put(int x, int y, char c) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        back_[y][x] = c;
}

void Renderer::putStr(int x, int y, const char* s) {
    for (int i = 0; s[i] && x + i < SCREEN_WIDTH; ++i)
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