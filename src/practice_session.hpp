#ifndef MATH_TRAINER_PRACTICE_SESSION_H
#define MATH_TRAINER_PRACTICE_SESSION_H

#include "answer_check.hpp"

#include <optional>
#include <random>

class PracticeSession {
public:
    explicit PracticeSession(NumberRange range);

    NumberRange range() const;
    Operation operation() const;
    const Task& currentTask() const;

    bool changeOperation(Operation operation);
    std::optional<AnswerStatus> submit(std::string_view input);
    bool nextTask();

private:
    NumberRange range_;
    Operation operation_ = Operation::Add;
    std::mt19937 random_{std::random_device{}()};
    Task task_;
    bool answered_ = false;
};

#endif
