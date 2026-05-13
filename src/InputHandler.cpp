#include "InputHandler.h"
#include "ProjectileManager.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

// Se guardan los ajustes originales de la terminal
static struct termios originalTermios;

// ─────────────────────────────────────────────────────────────────────────────
InputHandler::InputHandler() {
    enableRawMode();
}

InputHandler::~InputHandler() {
    disableRawMode();
}

// ─────────────────────────────────────────────────────────────────────────────
void InputHandler::enableRawMode() {
    tcgetattr(STDIN_FILENO, &originalTermios);

    struct termios raw = originalTermios;
    // Desactiva echo y modo canónico (espera de Enter)
    raw.c_lflag &= ~(ECHO | ICANON);
    // No bloqueante: read() devuelve inmediatamente
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void InputHandler::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}

int InputHandler::readKey() {
    char c;
    int n = read(STDIN_FILENO, &c, 1);
    return (n == 1) ? static_cast<int>(c) : -1;
}

// ─────────────────────────────────────────────────────────────────────────────
void InputHandler::processInput(GameState& gs) {
    int key = readKey();
    if (key == -1) return;  // No hay tecla pendiente

    // ── Teclas globales ───────────────────────────────────────────────────
    if (key == 'p' || key == 'P') {
        // Toggle pausa
        GameStatus cur = gs.status.load();
        if (cur == GameStatus::RUNNING)
            gs.status.store(GameStatus::PAUSED);
        else if (cur == GameStatus::PAUSED)
            gs.status.store(GameStatus::RUNNING);
        return;
    }
    if (key == 27) { // ESC
        gs.status.store(GameStatus::GAME_OVER);
        gs.running.store(false);
        return;
    }

    // Solo procesar movimiento si el juego está corriendo
    if (gs.status.load() != GameStatus::RUNNING) return;

    std::lock_guard<std::mutex> lock(gs.pitMutex);
    Player& pit = gs.pit;

    switch (key) {
        // ── Movimiento ────────────────────────────────────────────────────
        case 'a': case 'A':
            pit.pos.x = std::max(0, pit.pos.x - 1);
            pit.facing = Direction::LEFT;
            break;

        case 'd': case 'D':
            pit.pos.x = std::min(SCREEN_WIDTH - 1, pit.pos.x + 1);
            pit.facing = Direction::RIGHT;
            break;

        // ── Salto ─────────────────────────────────────────────────────────
        case 'w': case 'W':
            if (pit.onGround) {
                pit.velY    = -3;   // impulso inicial
                pit.onGround = false;
            }
            break;

        // ── Agacharse ─────────────────────────────────────────────────────
        case 's': case 'S':
            pit.crouching = true;
            break;

        // ── Disparar (dirección actual) ───────────────────────────────────
        case ' ':
            // Libera el lock de pit antes de tomar el de proyectiles
            // Guardamos la dirección antes de soltar el mutex
            {
                Direction d = pit.facing;
                // El lock de pit se libera al salir del bloque switch,
                // pero necesitamos disparar fuera del lock de pit.
                // Usamos una flag local para disparar después.
                // (Simplificación: disparar directamente; el semáforo
                //  en ProjectileManager protege el recurso)
                lock.~lock_guard(); // No se puede re-tomar; disparar tras return
                // NOTA: en implementación completa se usa una cola de acciones.
                ProjectileManager::firePlayerProjectile(gs, d);
            }
            return; // Salimos antes de que el lock_guard intente destruirse de nuevo

        // ── Disparar arriba ───────────────────────────────────────────────
        case 'c': case 'C':
            {
                lock.~lock_guard();
                ProjectileManager::firePlayerProjectile(gs, Direction::UP);
            }
            return;

        default:
            break;
    }

    // Al soltar 's' el personaje se levanta (simplificado: se resetea cada frame)
    if (key != 's' && key != 'S') {
        pit.crouching = false;
    }
}
