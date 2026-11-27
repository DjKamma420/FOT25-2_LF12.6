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

    if (input.front() == '+') {
        input.remove_prefix(1);
        if (input.empty() || input.front() == '-') {
            return AnswerStatus::Invalid;
        }
    }

    int value = 0;
    const char* end = input.data() + input.size();
    const auto parsed = std::from_chars(input.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return AnswerStatus::Invalid;
    }

    return value == task.answer ? AnswerStatus::Correct : AnswerStatus::Incorrect;
}
