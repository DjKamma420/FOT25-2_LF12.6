#include "practice_session.hpp"

#include "task_generator.hpp"

#include <utility>

PracticeSession::PracticeSession(NumberRange range, RoundMode mode, OperationSet operations)
    : range_(range), mode_(mode), operations_(operations), task_(makeTask()) {}

int PracticeSession::questionNumber() const {
    return questionNumber_;
}

TaskKind PracticeSession::currentKind() const {
    return mode_ == RoundMode::Endless ? selectTaskKind(questionNumber_) : TaskKind::Regular;
}

const Task& PracticeSession::currentTask() const {
    return task_;
}

const RunStats& PracticeSession::stats() const {
    return stats_;
}

bool PracticeSession::isDefeated() const {
    return mode_ == RoundMode::Endless && lastAnswer_ == AnswerStatus::Incorrect;
}

std::optional<AnswerStatus> PracticeSession::submit(std::string_view input, double seconds) {
    if (lastAnswer_) {
        return std::nullopt;
    }

    const auto status = checkAnswer(task_, input);
    if (status == AnswerStatus::Invalid) {
        return status;
    }

    if (status == AnswerStatus::Correct) {
        recordCorrectAnswer(stats_, currentKind(), seconds);
    } else {
        recordIncorrectAnswer(stats_);
    }
    lastAnswer_ = status;
    return status;
}

bool PracticeSession::nextTask() {
    if (!lastAnswer_ || isDefeated()) {
        return false;
    }

    ++questionNumber_;
    Task next = makeTask();
    while (next.expression == task_.expression) {
        next = makeTask();
    }
    task_ = std::move(next);
    lastAnswer_.reset();
    return true;
}

Task PracticeSession::makeTask() {
    if (currentKind() == TaskKind::Killer) {
        return generateKillerTask(range_, random_);
    }
    const auto operation = selectOperation(operations_, mode_, stats_.streak, random_);
    return generateTask(operation, range_, random_);
}
