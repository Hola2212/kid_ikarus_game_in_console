#pragma once

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#include "GameState.h"

class InputHandler {
private:
    struct termios originalTermios;

    void enableRawMode();
    void disableRawMode();

public:
    InputHandler();
    ~InputHandler();

    int readKey();

    void processInput(GameState& gs);
};