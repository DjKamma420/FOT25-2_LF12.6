#ifndef MATH_TRAINER_PRACTICE_SESSION_H
#define MATH_TRAINER_PRACTICE_SESSION_H

#include "answer_check.hpp"
#include "score.hpp"

#include <optional>
#include <random>

inline constexpr double killerTimeLimit = 60.0;

class PracticeSession {
public:
    PracticeSession();
    PracticeSession(NumberRange range, OperationMask operations, int decimalPlaces);

    int questionNumber() const;
    int currentNumberLimit() const;
    TaskKind currentKind() const;
    const Task& currentTask() const;
    const RunStats& stats() const;
    bool isEndless() const;
    bool isDefeated() const;
    bool hasTimedOut() const;

    std::optional<AnswerStatus> submit(std::string_view input, double seconds);
    bool expireIfNeeded(double seconds);
    bool nextTask();

private:
    Task makeTask();

    bool endless_ = true;
    NumberRange range_ = NumberRange::UpTo10;
    OperationMask operations_ = allOperations;
    int decimalPlaces_ = 1;
    int questionNumber_ = 1;
    RunStats stats_;
    std::mt19937 random_{std::random_device{}()};
    Task task_;
    std::optional<AnswerStatus> lastAnswer_;
    bool timedOut_ = false;
};

#endif
