#include "EnemyManager.h"
#include "ProjectileManager.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdlib>

// Ticker global para animaciones basadas en tiempo
static int gTick = 0;

// ─────────────────────────────────────────────────────────────────────────────
void EnemyManager::threadFunc(GameState* gs, const Level* level) {
    while (gs->running.load()) {
        if (gs->status.load() == GameStatus::RUNNING) {
            tick(*gs, *level);
            ++gTick;
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(ENEMY_THREAD_MS / speedForPhase(gs->phase.load()))
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void EnemyManager::tick(GameState& gs, const Level& level) {
    std::lock_guard<std::mutex> lock(gs.enemyMutex);

    // Necesitamos la posición de Pit (snapshot rápido)
    Player pitSnapshot;
    {
        std::lock_guard<std::mutex> pl(gs.pitMutex);
        pitSnapshot = gs.pit;
    }

    for (auto& e : gs.enemies) {
        if (!e.alive) continue;

        switch (e.type) {
            case EnemyType::MONOEYE:
                moveMonoeye(e, gs.phase.load());
                // Monoeye dispara ocasionalmente
                if (gTick % 15 == 0) {
                    Direction d = (pitSnapshot.pos.x < e.pos.x)
                                  ? Direction::LEFT : Direction::RIGHT;
                    ProjectileManager::fireEnemyProjectile(gs, e.pos, d);
                }
                break;

            case EnemyType::SHEMUM:
                moveShemum(e, level.getPlatforms());
                break;

            case EnemyType::REAPER:
                moveReaper(e, pitSnapshot, gs);
                break;

            case EnemyType::REAPETTE:
                moveReapette(e, pitSnapshot);
                break;

            case EnemyType::MEDUSA:
                moveMedusa(e, gTick);
                if (gTick % 8 == 0) {
                    ProjectileManager::fireEnemyProjectile(gs, e.pos, Direction::LEFT);
                    ProjectileManager::fireEnemyProjectile(gs, e.pos, Direction::RIGHT);
                }
                break;
        }
    }

    // Eliminar enemigos muertos (opcional: dejar corazones en el suelo)
    gs.enemies.erase(
        std::remove_if(gs.enemies.begin(), gs.enemies.end(),
                       [](const Enemy& e) { return !e.alive; }),
        gs.enemies.end()
    );
}

// ─── Patrones de movimiento ───────────────────────────────────────────────────
void EnemyManager::moveMonoeye(Enemy& e, int phase) {
    // Vuelo horizontal simple; rebota en los bordes
    e.pos.x += (e.dir == Direction::RIGHT ? 1 : -1);
    if (e.pos.x <= 0)                e.dir = Direction::RIGHT;
    if (e.pos.x >= SCREEN_WIDTH - 1) e.dir = Direction::LEFT;
    // Ligero movimiento vertical sinusoidal
    e.pos.y += (gTick % 6 == 0) ? 1 : (gTick % 3 == 0 ? -1 : 0);
    e.pos.y = std::max(1, std::min(SCREEN_HEIGHT - 2, e.pos.y));
}

void EnemyManager::moveShemum(Enemy& e, const std::vector<Level::Platform>& plats) {
    // Movimiento ondulatorio sobre plataformas
    e.pos.x += (e.dir == Direction::RIGHT ? 1 : -1);
    if (e.pos.x <= 0)                e.dir = Direction::RIGHT;
    if (e.pos.x >= SCREEN_WIDTH - 1) e.dir = Direction::LEFT;
    // Onda vertical simple
    e.pos.y += (gTick % 4 == 0) ? 1 : (gTick % 2 == 0 ? -1 : 0);
    e.pos.y = std::max(1, std::min(SCREEN_HEIGHT - 2, e.pos.y));
}

void EnemyManager::moveReaper(Enemy& e, const Player& pit, GameState& gs) {
    // Patrulla horizontal en zona fija (±5 celdas de su posición inicial)
    // Simplificado: usa pos.x como centro de patrulla
    static int patrolDir = 1;
    e.pos.x += patrolDir;
    if (e.pos.x % 5 == 0) patrolDir = -patrolDir;

    // Detecta a Pit en rango y lanza Reapettes
    int dx = std::abs(pit.pos.x - e.pos.x);
    int dy = std::abs(pit.pos.y - e.pos.y);
    if (dx <= 5 && dy <= 5 && gTick % 20 == 0) {
        spawnReapettes(e, gs);
    }
}

void EnemyManager::moveReapette(Enemy& e, const Player& pit) {
    // Se mueve directo hacia Pit
    if (pit.pos.x > e.pos.x) e.pos.x++;
    else if (pit.pos.x < e.pos.x) e.pos.x--;
    if (pit.pos.y > e.pos.y) e.pos.y++;
    else if (pit.pos.y < e.pos.y) e.pos.y--;
}

void EnemyManager::moveMedusa(Enemy& e, int tick) {
    // Medusa flota de lado a lado en la parte superior
    e.pos.x += (e.dir == Direction::RIGHT ? 1 : -1);
    if (e.pos.x <= 2)                e.dir = Direction::RIGHT;
    if (e.pos.x >= SCREEN_WIDTH - 3) e.dir = Direction::LEFT;
    // Descenso lento periódico
    if (tick % 30 == 0 && e.pos.y < SCREEN_HEIGHT / 2)
        e.pos.y++;
}

// ─────────────────────────────────────────────────────────────────────────────
void EnemyManager::spawnReapettes(Enemy& reaper, GameState& gs) {
    // Cuenta Reapettes activas para no superar 3 por Reaper
    int count = 0;
    for (const auto& e : gs.enemies)
        if (e.type == EnemyType::REAPETTE && e.alive) ++count;

    if (count >= 3) return;

    Enemy rp;
    rp.type          = EnemyType::REAPETTE;
    rp.pos           = {reaper.pos.x, reaper.pos.y - 1};
    rp.hp            = 1;
    rp.alive         = true;
    rp.heartsOnDeath = 0;
    gs.enemies.push_back(rp);
}

// ─────────────────────────────────────────────────────────────────────────────
int EnemyManager::speedForPhase(int phase) {
    // Factor multiplicador: fase 1 → 1x, fase 2 → 2x, fase 3 → 3x
    return std::max(1, phase);
}
