#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "GameState.h"

// Linux / WSL
#include <termios.h>
#include <unistd.h>

class InputHandler {
public:
    InputHandler();
    ~InputHandler();

    // Función principal
    void processInput(GameState& gs);

private:
    void enableRawMode();
    void disableRawMode();

    int readKey();

    struct termios originalTermios;
};

#endif