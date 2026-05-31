#include "InputHandler.h"
#include "ProjectileManager.h"
#include "Constants.h"

#include <cstdio>
#include <sys/ioctl.h>

InputHandler::InputHandler() {
    enableRawMode();
}

InputHandler::~InputHandler() {
    disableRawMode();
}

void InputHandler::enableRawMode() {

    tcgetattr(STDIN_FILENO, &originalTermios);

    struct termios raw = originalTermios;

    raw.c_lflag &= ~(ICANON | ECHO);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void InputHandler::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
}

int InputHandler::readKey() {

    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    int rv = select(STDIN_FILENO + 1,
                    &set,
                    NULL,
                    NULL,
                    &timeout);

    if (rv > 0) {

        unsigned char ch;

        if (read(STDIN_FILENO, &ch, 1) == 1)
            return ch;
    }

    return -1;
}

void InputHandler::processInput(GameState& gs) {

    int key;

    while ((key = readKey()) != -1) {

        GameStatus state = gs.status.load();

        // ==================================
        // ESC
        // ==================================

        if (key == 27) {

            // cerrar programa
            if (state == GameStatus::MENU ||
                state == GameStatus::GAME_OVER ||
                state == GameStatus::VICTORY ||
                state == GameStatus::SCORES ||
                state == GameStatus::INSTRUCTIONS) {

                gs.running.store(false);

                return;
            }

            // abandonar partida
            gs.status.store(GameStatus::MENU);

            continue;
        }

        // ==================================
        // PAUSA
        // ==================================

        if (key == 'p' || key == 'P') {

            if (state == GameStatus::RUNNING)
                gs.status.store(GameStatus::PAUSED);

            else if (state == GameStatus::PAUSED)
                gs.status.store(GameStatus::RUNNING);

            continue;
        }

        // ==================================
        // MENU
        // ==================================

        if (state == GameStatus::MENU) {

            if (key == '1' || key == '2') {

                gs.score.store(0);
                gs.phase.store(1);

                {
                    std::lock_guard<std::mutex> lock(gs.pitMutex);

                    gs.pit.pos.x = SCREEN_WIDTH / 2;
                    gs.pit.pos.y = GAME_HEIGHT - 2;

                    gs.pit.hp     = MAX_HP;
                    gs.pit.lives  = MAX_LIVES;
                    gs.pit.hearts = 0;

                    gs.pit.velY           = 0;
                    gs.pit.invincibleTicks = 0;

                    gs.pit.onGround  = true;
                    gs.pit.crouching = false;
                    gs.pit.facing    = Direction::RIGHT;
                }

                // Liberar semáforos de proyectiles que hayan quedado activos
                {
                    std::lock_guard<std::mutex> lock(gs.playerProjMutex);
                    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {
                        if (gs.playerProjs[i].active) {
                            gs.playerProjs[i].active = false;
                            gs.playerProjSem.release();
                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);
                    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {
                        if (gs.enemyProjs[i].active) {
                            gs.enemyProjs[i].active = false;
                            gs.enemyProjSem.release();
                        }
                    }
                }

                // GameLoop respawnea enemigos y cambia status a RUNNING
                gs.restartRequested.store(true);
            }

            else if (key == 'i' || key == 'I') {

                gs.status.store(GameStatus::INSTRUCTIONS);
            }

            else if (key == 's' || key == 'S') {

                gs.status.store(GameStatus::SCORES);
            }

            continue;
        }

        // ==================================
        // INSTRUCTIONS / SCORES
        // ==================================

        if (state == GameStatus::INSTRUCTIONS ||
            state == GameStatus::SCORES) {

            gs.status.store(GameStatus::MENU);

            continue;
        }

        // ==================================
        // GAME OVER / VICTORY
        // ==================================

        if (state == GameStatus::GAME_OVER ||
            state == GameStatus::VICTORY) {

            if (key == 'r' || key == 'R') {

                gs.score.store(0);
                gs.phase.store(1);

                {
                    std::lock_guard<std::mutex> lock(gs.pitMutex);

                    gs.pit.pos.x = SCREEN_WIDTH / 2;
                    gs.pit.pos.y = GAME_HEIGHT - 2;

                    gs.pit.hp = MAX_HP;
                    gs.pit.lives = MAX_LIVES;
                    gs.pit.hearts = 0;

                    gs.pit.velY = 0;
                    gs.pit.invincibleTicks = 0;

                    gs.pit.onGround = true;
                    gs.pit.crouching = false;

                    gs.pit.facing = Direction::RIGHT;
                }

                // Liberar semáforo por cada proyectil activo antes de limpiarlo
                {
                    std::lock_guard<std::mutex> lock(gs.playerProjMutex);

                    for (int i = 0; i < MAX_PLAYER_PROJ; ++i) {

                        if (gs.playerProjs[i].active) {
                            gs.playerProjs[i].active = false;
                            gs.playerProjSem.release();
                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(gs.enemyProjMutex);

                    for (int i = 0; i < MAX_ENEMY_PROJ; ++i) {

                        if (gs.enemyProjs[i].active) {
                            gs.enemyProjs[i].active = false;
                            gs.enemyProjSem.release();
                        }
                    }
                }

                // GameLoop se encarga de respawnear enemigos y cambiar status
                gs.restartRequested.store(true);
            }

            else if (key == 'm' || key == 'M') {

                gs.status.store(GameStatus::MENU);
            }

            continue;
        }

        // ==================================
        // SOLO GAMEPLAY
        // ==================================

        if (state != GameStatus::RUNNING)
            continue;

        switch (key) {

            // ==============================
            // SALTAR
            // ==============================

            case 'w':
            case 'W': {

                std::lock_guard<std::mutex> lock(gs.pitMutex);

                if (gs.pit.onGround) {

                    gs.pit.velY = -4;

                    gs.pit.onGround = false;
                }

                break;
            }

            // ==============================
            // AGACHARSE
            // ==============================

            case 's':
            case 'S': {

                std::lock_guard<std::mutex> lock(gs.pitMutex);

                // =========================================
                // Activar caída a través de plataforma
                // =========================================

                if (gs.pit.onGround) {

                    gs.pit.dropThroughTicks = 8;

                    gs.pit.onGround = false;

                    // Empujar ligeramente hacia abajo
                    gs.pit.pos.y += 1;
                }

                break;
            }

            // ==============================
            // IZQUIERDA
            // ==============================

            case 'a':
            case 'A': {

                std::lock_guard<std::mutex> lock(gs.pitMutex);

                gs.pit.pos.x--;

                if (gs.pit.pos.x < 0)
                    gs.pit.pos.x = 0;

                gs.pit.facing = Direction::LEFT;

                break;
            }

            // ==============================
            // DERECHA
            // ==============================

            case 'd':
            case 'D': {

                std::lock_guard<std::mutex> lock(gs.pitMutex);

                gs.pit.pos.x++;

                if (gs.pit.pos.x >= SCREEN_WIDTH)
                    gs.pit.pos.x = SCREEN_WIDTH - 1;

                gs.pit.facing = Direction::RIGHT;

                break;
            }

            // ==============================
            // DISPARAR
            // ==============================

            case ' ': {

                Direction dir;

                {
                    std::lock_guard<std::mutex> lock(gs.pitMutex);

                    dir = gs.pit.facing;
                }

                ProjectileManager::firePlayerProjectile(gs, dir);

                break;
            }

            // ==============================
            // DISPARAR ARRIBA
            // ==============================

            case 'c':
            case 'C': {

                ProjectileManager::firePlayerProjectile(
                    gs,
                    Direction::UP
                );

                break;
            }
        }
    }
}