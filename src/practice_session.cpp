#include "practice_session.hpp"

#include "task_generator.hpp"

#include <utility>

PracticeSession::PracticeSession(NumberRange range)
    : range_(range), task_(generateTask(operation_, range_, random_)) {}

NumberRange PracticeSession::range() const {
    return range_;
}

Operation PracticeSession::operation() const {
    return operation_;
}

const Task& PracticeSession::currentTask() const {
    return task_;
}

bool PracticeSession::changeOperation(Operation operation) {
    if (operation == operation_) {
        return false;
    }

    operation_ = operation;
    task_ = generateTask(operation_, range_, random_);
    answered_ = false;
    return true;
}

std::optional<AnswerStatus> PracticeSession::submit(std::string_view input) {
    if (answered_) {
        return std::nullopt;
    }

    const auto status = checkAnswer(task_, input);
    answered_ = status != AnswerStatus::Invalid;
    return status;
}

bool PracticeSession::nextTask() {
    if (!answered_) {
        return false;
    }

    Task next = generateTask(operation_, range_, random_);
    while (next.expression == task_.expression) {
        next = generateTask(operation_, range_, random_);
    }

    task_ = std::move(next);
    answered_ = false;
    return true;
}
