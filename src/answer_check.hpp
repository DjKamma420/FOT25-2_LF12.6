#ifndef MATH_TRAINER_ANSWER_CHECK_H
#define MATH_TRAINER_ANSWER_CHECK_H

#include "task.hpp"

#include <string>
#include <string_view>

enum class AnswerStatus {
    Correct,
    Incorrect,
    Invalid
};

AnswerStatus checkAnswer(const Task& task, std::string_view input);
std::string formatAnswer(std::int64_t answerUnits);

#endif
