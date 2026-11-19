#ifndef VAULT_HEIST_SCORE_SYSTEM_H
#define VAULT_HEIST_SCORE_SYSTEM_H

#include <algorithm>

class ScoreSystem {
public:
    static int alarmCapacity(int optimalSteps) {
        return std::max(1, (3 * std::max(0, optimalSteps) + 1) / 2 + 2);
    }
    static int score(int alarmSteps, int penalties) {
        return std::max(0, 1000 - 10 * alarmSteps - penalties);
    }
    static int stars(int alarmSteps, int optimalSteps, int penalties, int starCap = 3) {
        int earned = alarmSteps <= optimalSteps && penalties == 0 ? 3
                   : alarmSteps <= (5 * optimalSteps) / 4 && penalties <= 1 ? 2 : 1;
        return std::min(starCap, earned);
    }
};

#endif
