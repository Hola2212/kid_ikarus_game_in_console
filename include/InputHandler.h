#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "GameState.h"

class InputHandler {
public:
    InputHandler();

    // Procesa la entrada del usuario en tiempo real
    void processInput(GameState& gs);
};

#endif