#ifndef MATH_TRAINER_TASK_H
#define MATH_TRAINER_TASK_H

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

struct Task {
    std::string expression;
    int answerTenths;
    bool allowsDecimal = false;
};

#endif
