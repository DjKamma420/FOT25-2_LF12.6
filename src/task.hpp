#ifndef MATH_TRAINER_TASK_H
#define MATH_TRAINER_TASK_H

#include <cstdint>
#include <string>

enum class Operation {
    Add,
    Subtract,
    Multiply,
    Divide
};

enum class NumberRange {
    UpTo10 = 10,
    UpTo100 = 100,
    UpTo1000 = 1000
};

inline constexpr int answerScale = 1000;
inline constexpr int maximumDecimalPlaces = 3;

struct Task {
    std::string expression;
    std::int64_t answerUnits;
    int decimalPlaces = 0;
};

#endif
