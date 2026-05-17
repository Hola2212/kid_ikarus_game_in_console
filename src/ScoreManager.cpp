#include "ScoreManager.h"
#include <fstream>
#include <algorithm>

std::vector<ScoreEntry> ScoreManager::load() {
    std::vector<ScoreEntry> v;
    std::ifstream f("scores.txt");
    ScoreEntry e;
    while (f >> e.name >> e.score >> e.phase)
        v.push_back(e);
    return v;
}

bool ScoreManager::save(const std::string& name, int score, int phase) {
    auto v = load();
    v.push_back({name, score, phase});
    std::sort(v.begin(), v.end(), [](auto& a, auto& b){
        return a.score > b.score;
    });
    if ((int)v.size() > MAX_SCORES)
        v.resize(MAX_SCORES);
    std::ofstream f("scores.txt");
    for (auto& e : v)
        f << e.name << ' ' << e.score << ' ' << e.phase << '\n';
    for (auto& e : v)
        if (e.name == name && e.score == score) return true;
    return false;
}

bool ScoreManager::isHighScore(int score) {
    auto v = load();
    return (int)v.size() < MAX_SCORES || score > v.back().score;
}