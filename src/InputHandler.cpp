#include "InputHandler.h"
#include "ProjectileManager.h"
#include <cstdio>
#include <sys/ioctl.h>
// ─────────────────────────────
// Constructor / Destructor
// ─────────────────────────────
InputHandler::InputHandler() {
    enableRawMode();
}
InputHandler::~InputHandler() {
    disableRawMode();
}
// ─────────────────────────────
// RAW MODE (CLAVE)
// ─────────────────────────────
void InputHandler::enableRawMode() {
    tcgetattr(STDIN_FILENO, &originalTermios);
    struct termios raw = originalTermios;
    // Desactivar modo canónico y echo
    raw.c_lflag &= ~(ICANON | ECHO);
    // Lectura no bloqueante sin usar fcntl
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
void InputHandler::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
}
// ─────────────────────────────
// Leer tecla
// ─────────────────────────────
int InputHandler::readKey() {
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0; // no bloqueante
    int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    if (rv > 0) {
        unsigned char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1)
            return ch;
    }
    return -1;
}
// ─────────────────────────────
// INPUT PRINCIPAL (FIX REAL)
// ─────────────────────────────
void InputHandler::processInput(GameState& gs) {
    int key;
    // 🔥 Leer TODAS las teclas disponibles
    while ((key = readKey()) != -1) {
        // ───── GLOBAL ─────
        if (key == 27) { // ESC
            gs.running.store(false);
            gs.status.store(GameStatus::GAME_OVER);
            continue;
        }
        if (key == 'p' || key == 'P') {
            GameStatus cur = gs.status.load();
            if (cur == GameStatus::RUNNING)
                gs.status.store(GameStatus::PAUSED);
            else if (cur == GameStatus::PAUSED)
                gs.status.store(GameStatus::RUNNING);
            continue;
        }
        GameStatus state = gs.status.load();
        // ───── MENU ─────
        if (state == GameStatus::MENU) {
            if (key == '1' || key == '2') {
                gs.status.store(GameStatus::RUNNING);
            }
            else if (key == 'i' || key == 'I') { // NUEVO
                gs.status.store(GameStatus::INSTRUCTIONS);
            }
            else if (key == 's' || key == 'S') { // NUEVO
                gs.status.store(GameStatus::SCORES);
            }
            continue;
        }
        
        // ───── INSTRUCCIONES / SCORES — cualquier tecla vuelve al menu ─────
        if (state == GameStatus::INSTRUCTIONS || 
            state == GameStatus::SCORES) {       
            gs.status.store(GameStatus::MENU);
            continue;
        }
        // ───── GAME OVER / VICTORY ─────
        if (state == GameStatus::GAME_OVER || // NUEVO
            state == GameStatus::VICTORY) {   // NUEVO
            if (key == 'r' || key == 'R') {
                gs.status.store(GameStatus::MENU);
            }
            else if (key == 'm' || key == 'M') {
                gs.status.store(GameStatus::MENU);
            }
            continue;
        }
        // ───── SOLO EN JUEGO ─────
        if (state != GameStatus::RUNNING)
            continue;
        // ───── CONTROLES ─────
        switch (key) {
            case 'w': case 'W':
                // mover arriba — tarea de Juan Pablo
                break;
            case 's': case 'S':
                // mover abajo — tarea de Juan Pablo
                break;
            case 'a': case 'A':
                // mover izquierda — tarea de Juan Pablo
                break;
            case 'd': case 'D':
                // mover derecha — tarea de Juan Pablo
                break;
            case ' ': {
                // Disparar en la dirección que mira Pit
                ProjectileManager::firePlayerProjectile(gs, Direction::NONE);
                break;
            }
            case 'c': case 'C': {
                // Disparar hacia arriba
                ProjectileManager::firePlayerProjectile(gs, Direction::UP);
                break;
            }
        }
    }
}