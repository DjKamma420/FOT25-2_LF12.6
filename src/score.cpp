#include "score.hpp"

#include <algorithm>
#include <cmath>

double timeMultiplier(double seconds) {
    return 1.6 * std::clamp(1.0 - seconds / 10.0, 0.0, 1.0);
}

std::int64_t scoreMultiplierHundredths(const RunStats& stats, TaskKind kind, double seconds) {
    const bool isKiller = kind == TaskKind::Killer;
    const int level = std::clamp((stats.streak + 1) / 5, 0, 4);
    const int killerWins = stats.killerWinsInStreak + (isKiller ? 1 : 0);
    const double series = 1.0 + level * 0.25 + killerWins * 0.1;
    const double difficulty = isKiller ? 1.5 : 1.0;
    return std::llround(100.0 * difficulty * series * timeMultiplier(seconds));
}

std::int64_t recordCorrectAnswer(RunStats& stats, TaskKind kind, double seconds) {
    const auto multiplier = scoreMultiplierHundredths(stats, kind, seconds);
    ++stats.answered;
    ++stats.correct;
    ++stats.streak;
    stats.bestStreak = std::max(stats.bestStreak, stats.streak);
    stats.highestMultiplier = std::max(stats.highestMultiplier, multiplier);

    const bool isKiller = kind == TaskKind::Killer;
    if (isKiller) {
        ++stats.killerWinsInStreak;
        ++stats.killerWins;
    }
    // Base points are 100 for normal tasks and 200 for killer tasks.
    const auto points = multiplier * (isKiller ? 2 : 1);
    stats.score += points;
    return points;
}

void recordIncorrectAnswer(RunStats& stats) {
    ++stats.answered;
    stats.streak = 0;
    stats.killerWinsInStreak = 0;
}
