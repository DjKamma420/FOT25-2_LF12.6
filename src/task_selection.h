#ifndef MATH_TRAINER_TASK_SELECTION_H
#define MATH_TRAINER_TASK_SELECTION_H

#include "task.h"

#include <optional>

enum class TaskKind {
    Regular,
    Killer
};

std::optional<NumberRange> selectNumberRange(int limit);
TaskKind selectTaskKind(int questionNumber);

#endif
