#include "GameLoop.h"
#include <cstdio>

int main() {
    // Ocultar cursor para una visualización más limpia
    printf("\033[?25l");
    // Limpiar pantalla
    printf("\033[H\033[J");
    fflush(stdout);

    {
        GameLoop game;
        game.run();
    }

    // Restaurar cursor
    printf("\033[?25h\n");
    fflush(stdout);

    return 0;
}
