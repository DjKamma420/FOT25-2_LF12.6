#include "task_generator.hpp"

#include <algorithm>
#include <array>

namespace {

int randomNumber(int minimum, int maximum, std::mt19937& random) {
    std::uniform_int_distribution<int> distribution(minimum, maximum);
    return distribution(random);
}

}

Task generateTask(Operation operation, NumberRange range, std::mt19937& random) {
    const int limit = static_cast<int>(range);

    if (operation == Operation::Add) {
        const int first = randomNumber(1, limit - 1, random);
        const int second = randomNumber(1, limit - first, random);
        return {std::to_string(first) + " + " + std::to_string(second), (first + second) * 10};
    }

    if (operation == Operation::Multiply) {
        const int first = randomNumber(2, std::min(12, limit / 2), random);
        const int second = randomNumber(2, std::min(12, limit / first), random);
        return {std::to_string(first) + " × " + std::to_string(second), first * second * 10};
    }

    if (operation == Operation::Divide) {
        // These divisors always give an exact result in tenths.
        constexpr std::array divisors{2, 5, 10};
        const int divisor = divisors[randomNumber(0, 2, random)];
        const int dividend = randomNumber(1, limit, random);
        return {std::to_string(dividend) + " ÷ " + std::to_string(divisor), dividend * 10 / divisor, true};
    }

    const int first = randomNumber(1, limit, random);
    const int second = randomNumber(1, first, random);
    return {std::to_string(first) + " − " + std::to_string(second), (first - second) * 10};
}

Task generateKillerTask(NumberRange range, std::mt19937& random) {
    const int limit = static_cast<int>(range);
    const int termCount = randomNumber(3, 5, random);
    int total = randomNumber(1, limit, random);
    std::string expression = std::to_string(total);
    bool usedAddition = false;
    bool usedSubtraction = false;

    for (int index = 1; index < termCount; ++index) {
        bool add = total == 0 || (total < limit && randomNumber(0, 1, random) == 1);
        if (index == termCount - 1 && (!usedAddition || !usedSubtraction)) {
            add = !usedAddition;
        }
        if (add) {
            const int next = randomNumber(1, limit - total, random);
            expression += " + " + std::to_string(next);
            total += next;
            usedAddition = true;
        } else {
            const int next = randomNumber(1, total, random);
            expression += " − " + std::to_string(next);
            total -= next;
            usedSubtraction = true;
        }
    }

    return {expression, total * 10};
}
