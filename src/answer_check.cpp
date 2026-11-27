#include "answer_check.hpp"

#include <charconv>
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
        if (digits.empty() || digits.size() > static_cast<std::size_t>(task.decimalPlaces)) {
            return AnswerStatus::Invalid;
        }
        int place = answerScale / 10;
        for (const char digit : digits) {
            if (digit < '0' || digit > '9') {
                return AnswerStatus::Invalid;
            }
            fraction += (digit - '0') * place;
            place /= 10;
        }
        input = input.substr(0, decimal);
    }

    int whole = 0;
    const char* end = input.data() + input.size();
    const auto parsed = std::from_chars(input.data(), end, whole);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return AnswerStatus::Invalid;
    }

    const std::int64_t magnitude = static_cast<std::int64_t>(whole) * answerScale + fraction;
    const auto value = negative ? -magnitude : magnitude;
    return value == task.answerUnits ? AnswerStatus::Correct : AnswerStatus::Incorrect;
}

std::string formatAnswer(std::int64_t answerUnits) {
    const auto fraction = answerUnits % answerScale;
    std::string answer = std::to_string(answerUnits / answerScale);
    if (fraction != 0) {
        auto digits = std::to_string(fraction + answerScale).substr(1);
        digits.erase(digits.find_last_not_of('0') + 1);
        answer += "," + digits;
    }
    return answer;
}
