#include "practice_session.hpp"

#include "task_generator.hpp"

#include <utility>

PracticeSession::PracticeSession() : task_(makeTask()) {}

PracticeSession::PracticeSession(NumberRange range, OperationMask operations, int decimalPlaces)
    : endless_(false), range_(range), operations_(operations), decimalPlaces_(decimalPlaces), task_(makeTask()) {}

int PracticeSession::questionNumber() const {
    return questionNumber_;
}

int PracticeSession::currentNumberLimit() const {
    return endless_ ? endlessNumberLimit(questionNumber_ - 1) : static_cast<int>(range_);
}

TaskKind PracticeSession::currentKind() const {
    return endless_ ? selectTaskKind(questionNumber_) : TaskKind::Regular;
}

const Task& PracticeSession::currentTask() const {
    return task_;
}

const RunStats& PracticeSession::stats() const {
    return stats_;
}

bool PracticeSession::isEndless() const {
    return endless_;
}

bool PracticeSession::isDefeated() const {
    return endless_ && lastAnswer_ == AnswerStatus::Incorrect;
}

bool PracticeSession::hasTimedOut() const {
    return timedOut_;
}

bool PracticeSession::expireIfNeeded(double seconds) {
    if (lastAnswer_ || currentKind() != TaskKind::Killer || seconds < killerTimeLimit) {
        return false;
    }
    recordIncorrectAnswer(stats_);
    lastAnswer_ = AnswerStatus::Incorrect;
    timedOut_ = true;
    return true;
}

std::optional<AnswerStatus> PracticeSession::submit(std::string_view input, double seconds) {
    if (lastAnswer_) {
        return std::nullopt;
    }
    if (expireIfNeeded(seconds)) {
        return AnswerStatus::Incorrect;
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
    const int limit = currentNumberLimit();
    if (currentKind() == TaskKind::Killer) {
        const int terms = endlessTermCount(Operation::Add, stats_.streak);
        return generateKillerTask(limit, terms, random_);
    }

    if (!endless_) {
        return generateTask(selectOperation(operations_, random_), limit, decimalPlaces_, random_);
    }

    const auto operation = selectEndlessOperation(stats_.streak, random_);
    Task task = generateEndlessTask(operation, stats_.streak, random_);
    extendTask(task, limit, endlessTermCount(operation, stats_.streak) - 2, random_);
    return task;
}
