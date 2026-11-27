#include "task_generator.hpp"

#include "task_selection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

namespace {

int randomNumber(int minimum, int maximum, std::mt19937& random) {
    std::uniform_int_distribution<int> distribution(minimum, maximum);
    return distribution(random);
}

constexpr std::array powersOfTen{1, 10, 100, 1000};

Task multiplicationTask(int first, int second) {
    return {std::to_string(first) + " × " + std::to_string(second),
            static_cast<std::int64_t>(first) * second * answerScale};
}

Task divisionTask(int dividend, int divisor, int decimalPlaces) {
    return {std::to_string(dividend) + " ÷ " + std::to_string(divisor),
            static_cast<std::int64_t>(dividend) * answerScale / divisor, decimalPlaces};
}

Task generateDivision(int limit, int decimalPlaces, std::mt19937& random) {
    constexpr std::array factors{1, 2, 4, 5, 8, 10, 20, 25, 40, 50, 100, 125, 200, 250, 500, 1000};
    decimalPlaces = std::clamp(decimalPlaces, 0, maximumDecimalPlaces);
    const int allowedScale = powersOfTen[decimalPlaces];
    std::array<int, factors.size()> choices;
    int count = 0;
    for (const int factor : factors) {
        if (factor <= limit && allowedScale % factor == 0) {
            choices[count++] = factor;
        }
    }

    // A fraction with this denominator fits the selected decimal precision exactly.
    const int fractionDenominator = choices[randomNumber(0, count - 1, random)];
    const int multiplierMin = fractionDenominator == 1 ? 2 : 1;
    const int multiplierMax = std::min(limit / fractionDenominator, static_cast<int>(std::sqrt(limit)));
    const int multiplier = randomNumber(multiplierMin, multiplierMax, random);
    const int numerator = randomNumber(1, limit / multiplier, random);
    const int divisor = fractionDenominator * multiplier;
    const int dividend = numerator * multiplier;
    return divisionTask(dividend, divisor, decimalPlaces);
}

Task generateEndlessMultiplication(int limit, OperandDigits digits, std::mt19937& random) {
    const int firstMin = std::max(2, powersOfTen[digits.first - 1]);
    const int secondMin = std::max(2, powersOfTen[digits.second - 1]);
    const int firstMax = std::min(powersOfTen[digits.first] - 1, limit / secondMin);
    const int first = randomNumber(firstMin, firstMax, random);
    const int second = randomNumber(secondMin, std::min(powersOfTen[digits.second] - 1, limit / first), random);
    return multiplicationTask(first, second);
}

struct DivisionChoice {
    int divisor;
    int step;
    int firstMultiple;
    int lastMultiple;
    int shorterPeriod;
    int count;
};

Task generateEndlessDivision(int limit, OperandDigits digits, int decimalPlaces, std::mt19937& random) {
    const int dividendMin = powersOfTen[digits.first - 1];
    const int dividendMax = std::min(powersOfTen[digits.first] - 1, limit);
    const int divisorMin = std::max(2, powersOfTen[digits.second - 1]);
    const int divisorMax = powersOfTen[digits.second] - 1;
    const int scale = powersOfTen[decimalPlaces];
    std::vector<DivisionChoice> choices;
    choices.reserve(divisorMax - divisorMin + 1);

    for (int divisor = divisorMin; divisor <= divisorMax; ++divisor) {
        const int exactFactor = std::gcd(divisor, scale);
        const int step = divisor / exactFactor;
        const int firstMultiple = (dividendMin + step - 1) / step;
        const int lastMultiple = dividendMax / step;
        // Skip multiples that would give an answer with fewer decimal places.
        const int shorterPeriod = decimalPlaces == 0 ? 0 : exactFactor / std::gcd(divisor, scale / 10);
        const int shorterCount = shorterPeriod == 0
            ? 0 : lastMultiple / shorterPeriod - (firstMultiple - 1) / shorterPeriod;
        const int count = lastMultiple - firstMultiple + 1 - shorterCount;
        if (count > 0) {
            choices.push_back({divisor, step, firstMultiple, lastMultiple, shorterPeriod, count});
        }
    }

    if (choices.empty()) {
        return generateDivision(limit, decimalPlaces, random);
    }
    const auto& choice = choices[randomNumber(0, static_cast<int>(choices.size()) - 1, random)];
    int rank = randomNumber(1, choice.count, random);
    int multiple = choice.firstMultiple;
    for (; multiple < choice.lastMultiple; ++multiple) {
        if (choice.shorterPeriod > 0 && multiple % choice.shorterPeriod == 0) {
            continue;
        }
        if (--rank == 0) {
            break;
        }
    }
    return divisionTask(multiple * choice.step, choice.divisor, decimalPlaces);
}

bool chooseAddition(const Task& task, int limit, std::mt19937& random) {
    const auto limitUnits = static_cast<std::int64_t>(limit) * answerScale;
    if (task.answerUnits < answerScale) {
        return true;
    }
    return limitUnits - task.answerUnits >= answerScale && randomNumber(0, 1, random) == 1;
}

void appendTerm(Task& task, int limit, bool add, std::mt19937& random) {
    const auto available = add ? static_cast<std::int64_t>(limit) * answerScale - task.answerUnits
                               : task.answerUnits;
    const int next = randomNumber(1, static_cast<int>(available / answerScale), random);
    task.expression += (add ? " + " : " − ") + std::to_string(next);
    const auto units = static_cast<std::int64_t>(next) * answerScale;
    task.answerUnits += add ? units : -units;
}

}  // namespace

Task generateTask(Operation operation, int limit, int decimalPlaces, std::mt19937& random) {
    if (operation == Operation::Add) {
        const int first = randomNumber(1, limit - 1, random);
        const int second = randomNumber(1, limit - first, random);
        return {std::to_string(first) + " + " + std::to_string(second),
                static_cast<std::int64_t>(first + second) * answerScale};
    }

    if (operation == Operation::Multiply) {
        const int factorLimit = std::max(12, static_cast<int>(std::sqrt(limit)));
        const int first = randomNumber(2, std::min(factorLimit, limit / 2), random);
        const int second = randomNumber(2, std::min(factorLimit, limit / first), random);
        return multiplicationTask(first, second);
    }

    if (operation == Operation::Divide) {
        return generateDivision(limit, decimalPlaces, random);
    }

    const int first = randomNumber(1, limit, random);
    const int second = randomNumber(1, first, random);
    return {std::to_string(first) + " − " + std::to_string(second),
            static_cast<std::int64_t>(first - second) * answerScale};
}

Task generateEndlessTask(Operation operation, int streak, std::mt19937& random) {
    const int limit = endlessNumberLimit(streak);
    if (operation == Operation::Multiply) {
        return generateEndlessMultiplication(limit, multiplicationDigits(streak), random);
    }
    if (operation == Operation::Divide) {
        return generateEndlessDivision(limit, divisionDigits(streak), endlessDecimalPlaces(streak), random);
    }
    return generateTask(operation, limit, 1, random);
}

Task generateKillerTask(int limit, int regularTerms, std::mt19937& random) {
    const int termCount = regularTerms + randomNumber(1, 3, random);
    const int first = randomNumber(1, limit, random);
    Task task{std::to_string(first), static_cast<std::int64_t>(first) * answerScale};
    bool usedAddition = false;
    bool usedSubtraction = false;

    for (int index = 1; index < termCount; ++index) {
        bool add = chooseAddition(task, limit, random);
        if (index == termCount - 1 && (!usedAddition || !usedSubtraction)) {
            add = !usedAddition;
        }
        appendTerm(task, limit, add, random);
        usedAddition |= add;
        usedSubtraction |= !add;
    }
    return task;
}

void extendTask(Task& task, int limit, int additionalTerms, std::mt19937& random) {
    for (int index = 0; index < additionalTerms; ++index) {
        appendTerm(task, limit, chooseAddition(task, limit, random), random);
    }
}
