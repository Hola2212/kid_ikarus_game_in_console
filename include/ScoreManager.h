#pragma once
#include <string>
#include <vector>
#include "Constants.h"

struct ScoreEntry {
    std::string name;
    int score{0};
    int phase{1};
};

class ScoreManager {
public:
    static std::vector<ScoreEntry> load();
    static bool save(const std::string& name, int score, int phase);
    static bool isHighScore(int score);
};