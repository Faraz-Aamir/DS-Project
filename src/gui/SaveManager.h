#ifndef VAULT_HEIST_SAVE_MANAGER_H
#define VAULT_HEIST_SAVE_MANAGER_H

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class SaveManager {
public:
    explicit SaveManager(std::string leaderboardPath) : path(std::move(leaderboardPath)) {}
    void addScore(int score) const {
        std::ofstream file(path, std::ios::app);
        if (file) file << score << '\n';
    }
    std::vector<int> topScores() const {
        std::vector<int> scores;
        std::ifstream file(path);
        int score;
        while (file >> score) scores.push_back(score);
        std::sort(scores.begin(), scores.end(), std::greater<int>());
        if (scores.size() > 10) scores.resize(10);
        return scores;
    }
private:
    std::string path;
};

#endif
