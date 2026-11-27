#include "../src/practice_session.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/eventloop.h>
#include <emscripten/val.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <sstream>
#include <string>
#include <system_error>

EM_JS(double, readStoredNumber, (const char* key), {
    try {
        const value = Number(localStorage.getItem(UTF8ToString(key)));
        return Number.isSafeInteger(value) && value >= 0 ? value : 0;
    } catch {
        return 0;
    }
});

EM_JS(void, writeStoredNumber, (const char* key, double value), {
    try {
        localStorage.setItem(UTF8ToString(key), String(value));
    } catch {
        // Training is still available when browser storage is blocked.
    }
});

EM_JS(void, readEndlessRecord, (double* score, int* streak), {
    try {
        const record = JSON.parse(localStorage.getItem("rechentrainer.record.endless"));
        if (record && Number.isSafeInteger(record.score) && record.score >= 0
            && Number.isInteger(record.streak) && record.streak >= 0 && record.streak <= 2147483647) {
            HEAPF64[score >> 3] = record.score;
            HEAP32[streak >> 2] = record.streak;
        }
    } catch {
        // An unavailable or invalid record leaves the initialized values at zero.
    }
});

EM_JS(void, writeEndlessRecord, (double score, int streak), {
    try {
        localStorage.setItem("rechentrainer.record.endless", JSON.stringify({ score, streak }));
    } catch {
        // The current round remains playable when browser storage is blocked.
    }
});

namespace {

using emscripten::val;

constexpr std::array ranges{NumberRange::UpTo10, NumberRange::UpTo100, NumberRange::UpTo1000};
constexpr std::array screens{"home", "setup", "records", "practice", "defeat", "results"};
constexpr std::array operationChoices{
    OperationMask{1}, OperationMask{2}, OperationMask{4}, OperationMask{8}
};
constexpr std::array legacyOperationChoices{
    OperationMask{1}, OperationMask{2}, OperationMask{4}, OperationMask{8},
    additionAndSubtraction, OperationMask{12}, allOperations
};

std::optional<PracticeSession> session;
NumberRange range = NumberRange::UpTo100;
OperationMask practiceOperations = additionAndSubtraction;
int decimalPlaces = 1;
bool clockRunning = false;
double taskStart = 0.0;
double recordScore = 0.0;
int recordStreak = 0;

val element(const char* id) {
    return val::global("document").call<val>("getElementById", std::string(id));
}

void setText(const char* id, const std::string& text) {
    element(id).set("textContent", text);
}

void setHidden(const char* id, bool hidden) {
    element(id).set("hidden", hidden);
}

void setAttribute(const val& target, const char* name, const std::string& value) {
    target.call<void>("setAttribute", std::string(name), value);
}

std::optional<int> parseInt(const std::string& value) {
    int number = 0;
    const auto end = value.data() + value.size();
    const auto result = std::from_chars(value.data(), end, number);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return number;
}

std::string formatPoints(std::int64_t points) {
    std::string formatted = std::to_string(points);
    for (int pos = static_cast<int>(formatted.size()) - 3; pos > 0; pos -= 3) {
        formatted.insert(pos, ".");
    }
    return formatted;
}

std::string formatFactor(std::int64_t hundredths) {
    const auto fraction = hundredths % 100;
    return std::to_string(hundredths / 100) + "," + (fraction < 10 ? "0" : "") + std::to_string(fraction);
}

void refreshRecord() {
    double storedScore = 0.0;
    int storedStreak = 0;
    readEndlessRecord(&storedScore, &storedStreak);
    if (storedScore > recordScore || (storedScore == recordScore && storedStreak > recordStreak)) {
        recordScore = storedScore;
        recordStreak = storedStreak;
    }
}

void saveRecord() {
    if (!session->isEndless()) {
        return;
    }
    refreshRecord();
    const auto& stats = session->stats();
    if (stats.score > recordScore || (stats.score == recordScore && stats.bestStreak > recordStreak)) {
        recordScore = static_cast<double>(stats.score);
        recordStreak = stats.bestStreak;
        writeEndlessRecord(recordScore, recordStreak);
    }
}

void renderRecord() {
    refreshRecord();
    setText("record-score", formatPoints(static_cast<std::int64_t>(recordScore)));
    setText("record-streak", formatPoints(recordStreak));
}

void showScreen(const std::string& next) {
    for (const auto* id : screens) {
        setHidden(id, next != id);
    }
    setAttribute(element("game"), "data-screen", next);
    if (next != "practice") {
        setAttribute(element("game"), "data-killer", "false");
    }
    element((next + "-title").c_str()).call<void>("focus");
}

void updateSelection() {
    for (const auto candidate : ranges) {
        const auto id = "range-" + std::to_string(static_cast<int>(candidate));
        setAttribute(element(id.c_str()), "aria-pressed", candidate == range ? "true" : "false");
    }
    for (const auto choice : operationChoices) {
        const auto id = "operations-" + std::to_string(choice);
        element(id.c_str()).set("checked", (practiceOperations & choice) != 0);
    }
    setHidden("division-options", (practiceOperations & operationBit(Operation::Divide)) == 0);
    element("division-places").set("value", std::to_string(decimalPlaces));
    element("start-practice").set("disabled", practiceOperations == 0);
    setHidden("operation-warning", practiceOperations != 0);
}

double elapsedSeconds() {
    return (emscripten_get_now() - taskStart) / 1000.0;
}

void updateClock(double seconds, std::int64_t multiplier) {
    const bool killer = session->currentKind() == TaskKind::Killer;
    if (killer) {
        const auto left = std::ceil(std::max(0.0, killerTimeLimit - seconds));
        setText("timer", std::to_string(static_cast<int>(left)) + " s");
    } else {
        const auto tenths = std::lround(seconds * 10);
        setText("timer", std::to_string(tenths / 10) + "," + std::to_string(tenths % 10) + " s");
    }
    setText("live-multiplier", "×" + formatFactor(multiplier));
}

void showDefeat() {
    clockRunning = false;
    const auto& task = session->currentTask();
    setHidden("defeat-timeout", !session->hasTimedOut());
    setText("defeat-solution", task.expression + " = " + formatAnswer(task.answerUnits));
    showScreen("defeat");
}

void tickClock(void*) {
    if (!clockRunning) {
        return;
    }
    const double seconds = elapsedSeconds();
    updateClock(seconds, scoreMultiplierHundredths(session->stats(), session->currentKind(), seconds));
    if (session->expireIfNeeded(seconds)) {
        showDefeat();
    }
}

void updateHud() {
    const auto& stats = session->stats();
    setText("live-score", formatPoints(stats.score));
    setText("live-streak", formatPoints(stats.streak));
}

void setFeedback(const std::string& message, const std::string& state) {
    setText("feedback", message);
    setAttribute(element("feedback"), "data-state", state);
}

void renderExpression(const std::string& expression) {
    auto document = val::global("document");
    auto question = element("question");
    question.set("textContent", std::string());

    std::istringstream tokens(expression + " = ?");
    std::string token;
    int index = 0;
    while (tokens >> token) {
        if (index > 0) {
            question.call<val>("appendChild", document.call<val>("createTextNode", std::string(" ")));
        }
        auto span = document.call<val>("createElement", std::string("span"));
        if (index % 2 == 1) {
            span.set("className", std::string("operator"));
        }
        span.set("textContent", token);
        question.call<val>("appendChild", span);
        ++index;
    }
    setAttribute(question, "data-long", index > 7 ? "true" : "false");
}

std::string decimalLimitLabel(int places) {
    if (places == 1) {
        return "eine Nachkommastelle";
    }
    return std::to_string(places) + " Nachkommastellen";
}

void showQuestion() {
    const auto& task = session->currentTask();
    const bool killer = session->currentKind() == TaskKind::Killer;
    const bool endless = session->isEndless();
    const int number = session->questionNumber();
    const int phase = (number - 1) % 10 + 1;
    setAttribute(element("game"), "data-killer", killer ? "true" : "false");
    setHidden("killer-title", !killer);
    setHidden("progress-track", !endless);
    setHidden("progress-count", !endless);
    element("progress-track").set("value", phase);
    setText("progress-count", std::to_string(phase) + " / 10");
    setText("question-label", "Aufgabe " + formatPoints(number));
    setText("range-label", "Bis " + formatPoints(session->currentNumberLimit()));
    setText("timer-label", killer ? "Noch" : "Zeit");
    renderExpression(task.expression);
    setFeedback("", "");
    updateHud();

    auto answer = element("answer");
    answer.set("value", std::string());
    answer.set("readOnly", false);
    setAttribute(answer, "inputmode", task.decimalPlaces > 0 ? "decimal" : "numeric");
    answer.call<void>("removeAttribute", std::string("aria-invalid"));
    setHidden("decimal-hint", task.decimalPlaces == 0);
    if (task.decimalPlaces > 0) {
        setText("decimal-hint", "Höchstens " + decimalLimitLabel(task.decimalPlaces) + ".");
    }
    setHidden("check-button", false);
    setHidden("next-button", true);
    taskStart = emscripten_get_now();
    clockRunning = true;
    updateClock(0.0, scoreMultiplierHundredths(session->stats(), session->currentKind(), 0.0));
    answer.call<void>("focus");
}

void showResults() {
    clockRunning = false;
    const auto& stats = session->stats();
    const std::string modeLabel = session->isEndless() ? "Endlos · " : "Üben · ";
    setText("result-mode", modeLabel + "Bis " + formatPoints(session->currentNumberLimit()));
    setText("result-correct", formatPoints(stats.correct) + " / " + formatPoints(stats.answered));
    setText("result-streak", formatPoints(stats.bestStreak));
    setText("result-score", formatPoints(stats.score));
    showScreen("results");
}

void handleNavigation(val event) {
    const auto target = event["currentTarget"]["value"].as<std::string>();
    if (target == "records") {
        renderRecord();
    }
    if (target == "home") {
        clockRunning = false;
        session.reset();
    }
    showScreen(target);
}

void handleOperationChoice(val event) {
    const auto input = event["currentTarget"];
    const auto value = parseInt(input["value"].as<std::string>());
    if (!value) {
        return;
    }

    const auto choice = static_cast<OperationMask>(*value);
    if (input["checked"].as<bool>()) {
        practiceOperations |= choice;
    } else {
        practiceOperations &= ~choice;
    }
    writeStoredNumber("rechentrainer.practice-operations", practiceOperations + 1);
    updateSelection();
}

void handleRangeChoice(val event) {
    const auto value = parseInt(event["currentTarget"]["value"].as<std::string>());
    if (value) {
        if (const auto selected = selectNumberRange(*value)) {
            range = *selected;
            writeStoredNumber("rechentrainer.practice-range", *value);
            updateSelection();
        }
    }
}

void handleDecimalChoice(val event) {
    const auto value = parseInt(event["currentTarget"]["value"].as<std::string>());
    if (value && *value >= 0 && *value <= maximumDecimalPlaces) {
        decimalPlaces = *value;
        writeStoredNumber("rechentrainer.division-places", decimalPlaces + 1);
    }
}

void startRound(bool endless) {
    if (endless) {
        session.emplace();
    } else {
        if (practiceOperations == 0) {
            return;
        }
        session.emplace(range, practiceOperations, decimalPlaces);
    }
    setText("practice-title", endless ? "Endlosrunde" : "Übungsrunde");
    showScreen("practice");
    showQuestion();
}

void handleStart(val event) {
    const auto mode = event["currentTarget"]["value"].as<std::string>();
    if (mode == "restart" && !session) {
        return;
    }
    startRound(mode == "endless" || (mode == "restart" && session->isEndless()));
}

void handleAnswerSubmit(val event) {
    event.call<void>("preventDefault");
    if (!session || !clockRunning) {
        return;
    }

    auto input = element("answer");
    const double seconds = elapsedSeconds();
    const auto previousScore = session->stats().score;
    const auto multiplier = scoreMultiplierHundredths(session->stats(), session->currentKind(), seconds);
    const auto answer = session->submit(input["value"].as<std::string>(), seconds);
    if (!answer) {
        return;
    }

    const auto& task = session->currentTask();
    if (*answer == AnswerStatus::Invalid) {
        const auto message = task.decimalPlaces > 0
            ? "Gib eine Zahl mit höchstens " + decimalLimitLabel(task.decimalPlaces) + " ein."
            : "Gib eine ganze Zahl ein.";
        setFeedback(message, "invalid");
        setAttribute(input, "aria-invalid", "true");
        input.call<void>("focus");
        input.call<void>("select");
        return;
    }

    clockRunning = false;
    updateClock(seconds, *answer == AnswerStatus::Correct ? multiplier : 0);
    updateHud();
    saveRecord();
    if (session->isDefeated()) {
        showDefeat();
        return;
    }

    input.call<void>("removeAttribute", std::string("aria-invalid"));
    input.set("readOnly", true);
    if (*answer == AnswerStatus::Correct) {
        const auto points = formatPoints(session->stats().score - previousScore);
        setFeedback("Richtig! +" + points + " Punkte", "correct");
    } else {
        setFeedback("Die richtige Lösung ist " + formatAnswer(task.answerUnits) + ".", "incorrect");
    }
    setHidden("check-button", true);
    setHidden("next-button", false);
    element("next-button").call<void>("focus");
}

void handleNext(val) {
    if (session && session->nextTask()) {
        showQuestion();
    }
}

void handleEnd(val) {
    if (!session) {
        return;
    }
    if (clockRunning && session->expireIfNeeded(elapsedSeconds())) {
        showDefeat();
        return;
    }
    showResults();
}

void bindGroup(const char* selector, const char* event, const char* handler) {
    auto targets = val::global("document").call<val>("querySelectorAll", std::string(selector));
    const int count = targets["length"].as<int>();
    for (int i = 0; i < count; ++i) {
        targets[i].set(event, val::module_property(handler));
    }
}

}  // namespace

EMSCRIPTEN_BINDINGS(math_trainer) {
    emscripten::function("handleNavigation", &handleNavigation);
    emscripten::function("handleOperationChoice", &handleOperationChoice);
    emscripten::function("handleRangeChoice", &handleRangeChoice);
    emscripten::function("handleDecimalChoice", &handleDecimalChoice);
    emscripten::function("handleStart", &handleStart);
    emscripten::function("handleAnswerSubmit", &handleAnswerSubmit);
    emscripten::function("handleNext", &handleNext);
    emscripten::function("handleEnd", &handleEnd);
}

int main() {
    const double storedMask = readStoredNumber("rechentrainer.practice-operations");
    if (storedMask >= 1 && storedMask <= allOperations + 1) {
        practiceOperations = static_cast<OperationMask>(storedMask) - 1;
    } else {
        const double legacy = readStoredNumber("rechentrainer.operations");
        if (legacy >= 1 && legacy <= legacyOperationChoices.size()) {
            practiceOperations = legacyOperationChoices[static_cast<int>(legacy) - 1];
        }
    }
    const double storedPlaces = readStoredNumber("rechentrainer.division-places");
    if (storedPlaces >= 1 && storedPlaces <= maximumDecimalPlaces + 1) {
        decimalPlaces = static_cast<int>(storedPlaces) - 1;
    }
    const auto storedRange = readStoredNumber("rechentrainer.practice-range");
    if (storedRange == 10 || storedRange == 100 || storedRange == 1000) {
        range = static_cast<NumberRange>(static_cast<int>(storedRange));
    }

    bindGroup("[data-nav]", "onclick", "handleNavigation");
    bindGroup("[data-range]", "onclick", "handleRangeChoice");
    bindGroup("[data-start]", "onclick", "handleStart");
    bindGroup("[data-end]", "onclick", "handleEnd");
    bindGroup("[name=operations]", "onchange", "handleOperationChoice");
    element("division-places").set("onchange", val::module_property("handleDecimalChoice"));
    element("answer-form").set("onsubmit", val::module_property("handleAnswerSubmit"));
    element("next-button").set("onclick", val::module_property("handleNext"));
    element("home-actions").set("disabled", false);
    setHidden("loading-status", true);
    updateSelection();
    emscripten_set_interval(tickClock, 125.0, nullptr);
}
