#ifndef MATH_TRAINER_PRACTICE_SESSION_H
#define MATH_TRAINER_PRACTICE_SESSION_H

#include "answer_check.hpp"
#include "score.hpp"

#include <optional>
#include <random>

class PracticeSession {
public:
    PracticeSession(NumberRange range, RoundMode mode, OperationSet operations);

    int questionNumber() const;
    TaskKind currentKind() const;
    const Task& currentTask() const;
    const RunStats& stats() const;
    bool isDefeated() const;

    std::optional<AnswerStatus> submit(std::string_view input, double seconds);
    bool nextTask();

private:
    Task makeTask();

    NumberRange range_;
    RoundMode mode_;
    OperationSet operations_;
    int questionNumber_ = 1;
    RunStats stats_;
    std::mt19937 random_{std::random_device{}()};
    Task task_;
    std::optional<AnswerStatus> lastAnswer_;
};

#endif
