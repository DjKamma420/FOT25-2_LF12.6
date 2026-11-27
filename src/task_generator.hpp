#ifndef MATH_TRAINER_TASK_GENERATOR_H
#define MATH_TRAINER_TASK_GENERATOR_H

#include "task.hpp"

#include <random>

Task generateTask(Operation operation, int limit, int decimalPlaces, std::mt19937& random);
Task generateEndlessTask(Operation operation, int streak, std::mt19937& random);
Task generateKillerTask(int limit, int regularTerms, std::mt19937& random);
void extendTask(Task& task, int limit, int additionalTerms, std::mt19937& random);

#endif
