#ifndef MATH_TRAINER_TASK_SELECTION_H
#define MATH_TRAINER_TASK_SELECTION_H

#include "task.hpp"

#include <optional>
#include <random>

using OperationMask = unsigned;

constexpr OperationMask operationBit(Operation operation) {
    return 1U << static_cast<unsigned>(operation);
}

inline constexpr OperationMask additionAndSubtraction =
    operationBit(Operation::Add) | operationBit(Operation::Subtract);
inline constexpr OperationMask allOperations =
    additionAndSubtraction | operationBit(Operation::Multiply) | operationBit(Operation::Divide);

enum class TaskKind {
    Regular,
    Killer
};

struct OperandDigits {
    int first;
    int second;
};

std::optional<NumberRange> selectNumberRange(int limit);
TaskKind selectTaskKind(int questionNumber);
Operation selectOperation(OperationMask operations, std::mt19937& random);
Operation selectEndlessOperation(int streak, std::mt19937& random);
int endlessNumberLimit(int streak);
int endlessTermCount(Operation operation, int streak);
int endlessDecimalPlaces(int streak);
OperandDigits multiplicationDigits(int streak);
OperandDigits divisionDigits(int streak);

#endif
