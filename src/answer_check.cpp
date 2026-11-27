#include "answer_check.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

AnswerStatus checkAnswer(const Task& task, std::string_view input) {
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    const auto first = input.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return AnswerStatus::Invalid;
    }

    const auto last = input.find_last_not_of(whitespace);
    input = input.substr(first, last - first + 1);
    const bool negative = input.front() == '-';
    if (negative || input.front() == '+') {
        input.remove_prefix(1);
    }
    if (input.empty() || input.front() < '0' || input.front() > '9') {
        return AnswerStatus::Invalid;
    }

    int fraction = 0;
    const auto decimal = input.find_first_of(".,");
    if (decimal != std::string_view::npos) {
        const auto digits = input.substr(decimal + 1);
        if (!task.allowsDecimal || digits.size() != 1 || digits.front() < '0' || digits.front() > '9') {
            return AnswerStatus::Invalid;
        }
        fraction = digits.front() - '0';
        input = input.substr(0, decimal);
    }

    int whole = 0;
    const char* end = input.data() + input.size();
    const auto parsed = std::from_chars(input.data(), end, whole);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return AnswerStatus::Invalid;
    }

    const std::int64_t magnitude = static_cast<std::int64_t>(whole) * 10 + fraction;
    const auto value = negative ? -magnitude : magnitude;
    return value == task.answerTenths ? AnswerStatus::Correct : AnswerStatus::Incorrect;
}

std::string formatAnswer(int answerTenths) {
    const int fraction = answerTenths % 10;
    std::string answer = std::to_string(answerTenths / 10);
    if (fraction != 0) {
        answer += "," + std::to_string(fraction);
    }
    return answer;
}
