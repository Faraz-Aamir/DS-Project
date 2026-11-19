#ifndef VAULT_HEIST_LEVEL_MANAGER_H
#define VAULT_HEIST_LEVEL_MANAGER_H

#include "LevelLoader.h"
#include <string>
#include <unordered_set>

class LevelManager {
public:
    LevelLoader levels;
    int actIndex = 0;
    int levelIndex = 0;

    bool load(const std::string& path) { return levels.load(path); }
    int totalLevels() const {
        int count = 0;
        for (const Act& act : levels.acts) count += (int)act.levels.size();
        return count;
    }
    bool unlocked(int act, int level, const std::unordered_set<std::string>& completed) const {
        if (act < 0 || act >= (int)levels.acts.size() || level < 0 ||
            level >= (int)levels.acts[act].levels.size()) return false;
        if (act == 0 && level == 0) return true;
        int previousAct = act, previousLevel = level - 1;
        if (previousLevel < 0) { --previousAct; if (previousAct < 0) return false;
            previousLevel = (int)levels.acts[previousAct].levels.size() - 1; }
        return completed.count(levels.acts[previousAct].levels[previousLevel].id) != 0;
    }
};

#endif
