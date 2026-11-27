#include "task_selection.h"

std::optional<NumberRange> selectNumberRange(int limit) {
    switch (limit) {
        case 10: return NumberRange::UpTo10;
        case 100: return NumberRange::UpTo100;
        case 1000: return NumberRange::UpTo1000;
        default: return std::nullopt;
    }
}

TaskKind selectTaskKind(int questionNumber) {
    if (questionNumber > 0 && questionNumber % 10 == 0) {
        return TaskKind::Killer;
    }
    return TaskKind::Regular;
}
