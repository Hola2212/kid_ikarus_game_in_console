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

                {
                    std::lock_guard<std::mutex> lock(gs.pitMutex);

                    gs.pit.pos.x = SCREEN_WIDTH / 2;
                    gs.pit.pos.y = GAME_HEIGHT - 2;

                    gs.pit.hp = MAX_HP;
                    gs.pit.lives = MAX_LIVES;

                    gs.pit.velY = 0;

                    gs.pit.onGround = true;
                }

                gs.status.store(GameStatus::RUNNING);
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

                {
                    std::lock_guard<std::mutex> lock(gs.pitMutex);

                    gs.pit.pos.x = SCREEN_WIDTH / 2;
                    gs.pit.pos.y = GAME_HEIGHT - 2;

                    gs.pit.hp = MAX_HP;
                    gs.pit.lives = MAX_LIVES;

                    gs.pit.velY = 0;

                    gs.pit.onGround = true;
                }

                gs.status.store(GameStatus::RUNNING);
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

                    gs.pit.velY = -3;

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

                gs.pit.crouching = true;

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