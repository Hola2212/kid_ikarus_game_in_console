#pragma once
#include <vector>
#include "Constants.h"
#include "GameState.h"

// ─────────────────────────────────────────────────────────────────────────────
// Level — Gestiona plataformas y spawn de enemigos para cada fase del nivel.
// ─────────────────────────────────────────────────────────────────────────────
class Level {
public:
    // Plataformas: lista de posiciones (x, y) + longitud
    struct Platform {
        int x, y, length;
    };

    explicit Level(int phase);

    // Construye la lista de plataformas para la fase actual
    const std::vector<Platform>& getPlatforms() const { return platforms_; }

    // Devuelve los enemigos iniciales de la fase (sin Medusa)
    std::vector<Enemy> spawnEnemies() const;

    // Devuelve el enemigo jefe (Medusa), solo en fase 3 tras limpiar pantalla
    Enemy spawnMedusa() const;

    // ¿Hay tienda antes de entrar a esta fase?
    bool hasShopBefore() const { return phase_ == 3; }

    int getPhase() const { return phase_; }

private:
    int                   phase_;
    std::vector<Platform> platforms_;

    void buildPhase1();
    void buildPhase2();
    void buildPhase3();
};
