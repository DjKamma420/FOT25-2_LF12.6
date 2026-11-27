#ifndef MATH_TRAINER_SCORE_H
#define MATH_TRAINER_SCORE_H

#include "task_selection.hpp"

#include <cstdint>

struct RunStats {
    int answered = 0;
    int correct = 0;
    int streak = 0;
    int bestStreak = 0;
    int killerWinsInStreak = 0;
    int killerWins = 0;
    std::int64_t score = 0;
    std::int64_t highestMultiplier = 0;
};

double timeMultiplier(double seconds);
std::int64_t scoreMultiplierHundredths(const RunStats& stats, TaskKind kind, double seconds);
std::int64_t recordCorrectAnswer(RunStats& stats, TaskKind kind, double seconds);
void recordIncorrectAnswer(RunStats& stats);

#endif
