#include "score.hpp"

#include <algorithm>
#include <cmath>

double timeMultiplier(double seconds) {
    seconds = std::max(0.0, seconds);
    if (seconds < 3.0) {
        return 1.6 - seconds * (0.2 / 3.0);
    }
    if (seconds < 6.0) {
        return 1.3 - (seconds - 3.0) * (0.2 / 3.0);
    }
    if (seconds <= 10.0) {
        return 1.1 - (seconds - 6.0) * (0.2 / 4.0);
    }
    return std::max(0.6, 0.8 - (seconds - 10.0) * 0.02);
}

double streakMultiplier(const RunStats& stats) {
    const int level = std::clamp(stats.streak / 5, 0, 4);
    return 1.0 + level * 0.25 + stats.killerWinsInStreak * 0.1;
}

std::int64_t recordCorrectAnswer(RunStats& stats, TaskKind kind, double seconds) {
    ++stats.answered;
    ++stats.correct;
    ++stats.streak;
    stats.bestStreak = std::max(stats.bestStreak, stats.streak);

    const bool isKiller = kind == TaskKind::Killer;
    if (isKiller) {
        ++stats.killerWinsInStreak;
    }
    const int basePoints = isKiller ? 200 : 100;
    const double difficulty = isKiller ? 1.5 : 1.0;
    const auto points = std::llround(basePoints * difficulty * streakMultiplier(stats) * timeMultiplier(seconds));
    stats.score += points;
    return points;
}

void recordIncorrectAnswer(RunStats& stats) {
    ++stats.answered;
    stats.streak = 0;
    stats.killerWinsInStreak = 0;
}
