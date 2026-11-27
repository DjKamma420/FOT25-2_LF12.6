#include "task_generator.hpp"

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
        return {std::to_string(first) + " + " + std::to_string(second), first + second};
    }

    const int first = randomNumber(1, limit, random);
    const int second = randomNumber(1, first, random);
    return {std::to_string(first) + " - " + std::to_string(second), first - second};
}

Task generateKillerTask(NumberRange range, std::mt19937& random) {
    const int limit = static_cast<int>(range);
    const int termCount = randomNumber(3, 5, random);
    int total = randomNumber(1, limit, random);
    std::string expression = std::to_string(total);

    for (int index = 1; index < termCount; ++index) {
        const bool add = total == 0 || (total < limit && randomNumber(0, 1, random) == 1);
        if (add) {
            const int next = randomNumber(1, limit - total, random);
            expression += " + " + std::to_string(next);
            total += next;
        } else {
            const int next = randomNumber(1, total, random);
            expression += " - " + std::to_string(next);
            total -= next;
        }
    }

    return {expression, total};
}
