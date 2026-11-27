#ifndef MATH_TRAINER_TASK_GENERATOR_H
#define MATH_TRAINER_TASK_GENERATOR_H

#include "task.h"

#include <random>

Task generateTask(Operation operation, NumberRange range, std::mt19937& random);
Task generateKillerTask(NumberRange range, std::mt19937& random);

#endif
