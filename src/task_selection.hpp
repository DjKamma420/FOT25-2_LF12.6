#ifndef MATH_TRAINER_TASK_SELECTION_H
#define MATH_TRAINER_TASK_SELECTION_H

#include "task.hpp"

#include <optional>
#include <random>

enum class RoundMode {
    Practice,
    Endless
};

enum class OperationSet {
    Addition,
    Subtraction,
    Multiplication,
    Division,
    AddSubtract,
    MultiplyDivide,
    All
};

enum class TaskKind {
    Regular,
    Killer
};

std::optional<NumberRange> selectNumberRange(int limit);
TaskKind selectTaskKind(int questionNumber);
Operation selectOperation(OperationSet operations, RoundMode mode, int streak, std::mt19937& random);

#endif
