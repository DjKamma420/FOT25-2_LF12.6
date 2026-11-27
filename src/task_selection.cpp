#include "task_selection.hpp"

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

Operation selectOperation(OperationSet operations, RoundMode mode, int streak, std::mt19937& random) {
    int firstOperation = 0;
    int lastOperation = 3;
    switch (operations) {
        case OperationSet::Addition: return Operation::Add;
        case OperationSet::Subtraction: return Operation::Subtract;
        case OperationSet::Multiplication: return Operation::Multiply;
        case OperationSet::Division: return Operation::Divide;
        case OperationSet::AddSubtract:
            lastOperation = 1;
            break;
        case OperationSet::MultiplyDivide:
            firstOperation = 2;
            break;
        case OperationSet::All:
            if (mode == RoundMode::Endless) {
                if (streak == 8) {
                    return Operation::Multiply;
                }
                if (streak == 15) {
                    return Operation::Divide;
                }
                lastOperation = streak >= 15 ? 3 : streak >= 8 ? 2 : 1;
            }
            break;
    }
    std::uniform_int_distribution<int> distribution(firstOperation, lastOperation);
    return static_cast<Operation>(distribution(random));
}
