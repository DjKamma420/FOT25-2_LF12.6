#include "task_selection.hpp"

#include <algorithm>
#include <array>

namespace {

constexpr int multiplicationUnlock = 20;
constexpr int divisionUnlock = 50;
constexpr int maximumRangeStreak = 99;  // Question 100 follows 99 correct answers.
constexpr int maximumDifficultyStreak = 300;
constexpr int maximumTerms = 14;

struct DigitStage {
    int streak;
    OperandDigits digits;
};

constexpr std::array multiplicationStages{
    DigitStage{multiplicationUnlock, {1, 1}},
    DigitStage{50, {2, 1}},
    DigitStage{100, {2, 2}}
};

constexpr std::array divisionStages{
    DigitStage{divisionUnlock, {2, 1}},
    DigitStage{100, {3, 1}},
    DigitStage{125, {3, 2}},
    DigitStage{150, {3, 3}}
};

template <std::size_t size>
OperandDigits digitsAt(const std::array<DigitStage, size>& stages, int streak) {
    auto digits = stages.front().digits;
    for (const auto& stage : stages) {
        if (streak < stage.streak) {
            break;
        }
        digits = stage.digits;
    }
    return digits;
}

}  // namespace

std::optional<NumberRange> selectNumberRange(int limit) {
    switch (limit) {
        case 10: return NumberRange::UpTo10;
        case 100: return NumberRange::UpTo100;
        case 1000: return NumberRange::UpTo1000;
        default: return std::nullopt;
    }
}

TaskKind selectTaskKind(int questionNumber) {
    return questionNumber > 0 && questionNumber % 10 == 0 ? TaskKind::Killer : TaskKind::Regular;
}

Operation selectOperation(OperationMask operations, std::mt19937& random) {
    constexpr std::array supported{Operation::Add, Operation::Subtract, Operation::Multiply, Operation::Divide};
    std::array<Operation, supported.size()> enabled;
    int count = 0;
    for (const auto operation : supported) {
        if (operations & operationBit(operation)) {
            enabled[count++] = operation;
        }
    }
    if (count == 0) {
        return Operation::Add;
    }
    return enabled[std::uniform_int_distribution<int>(0, count - 1)(random)];
}

Operation selectEndlessOperation(int streak, std::mt19937& random) {
    if (streak == multiplicationUnlock) {
        return Operation::Multiply;
    }
    if (streak == divisionUnlock) {
        return Operation::Divide;
    }
    if (streak >= divisionUnlock) {
        constexpr std::array weightedOperations{
            Operation::Add, Operation::Add, Operation::Subtract, Operation::Subtract,
            Operation::Multiply, Operation::Multiply, Operation::Divide
        };
        return weightedOperations[std::uniform_int_distribution<int>(0, weightedOperations.size() - 1)(random)];
    }
    const auto operations = streak >= multiplicationUnlock
        ? additionAndSubtraction | operationBit(Operation::Multiply) : additionAndSubtraction;
    return selectOperation(operations, random);
}

int endlessNumberLimit(int streak) {
    if (streak < 10) {
        return 10;
    }
    if (streak < maximumRangeStreak) {
        return 100;
    }

    return 1000;
}

int endlessTermCount(Operation operation, int streak) {
    // Longer expressions begin once the base operands have reached full size.
    int firstStreak = maximumRangeStreak;
    if (operation == Operation::Multiply) {
        firstStreak = multiplicationStages.back().streak;
    } else if (operation == Operation::Divide) {
        firstStreak = divisionStages.back().streak;
    }
    const int progress = std::clamp(streak, firstStreak, maximumDifficultyStreak) - firstStreak;
    return 2 + progress * (maximumTerms - 2) / (maximumDifficultyStreak - firstStreak);
}

int endlessDecimalPlaces(int streak) {
    return std::clamp(streak / 100, 0, maximumDecimalPlaces);
}

OperandDigits multiplicationDigits(int streak) {
    return digitsAt(multiplicationStages, streak);
}

OperandDigits divisionDigits(int streak) {
    return digitsAt(divisionStages, streak);
}
