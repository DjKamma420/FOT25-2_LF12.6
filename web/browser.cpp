#include "../src/practice_session.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/eventloop.h>
#include <emscripten/val.h>

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

namespace {

using emscripten::val;

constexpr std::array operationLabels{
    "Plus", "Minus", "Mal", "Geteilt", "Gemischt Plus/Minus",
    "Gemischt Mal/Geteilt", "Alles"
};
constexpr std::array ranges{NumberRange::UpTo10, NumberRange::UpTo100, NumberRange::UpTo1000};
constexpr std::array screens{"home", "setup", "settings", "records", "practice", "defeat", "results"};

std::optional<PracticeSession> session;
NumberRange range = NumberRange::UpTo100;
RoundMode roundMode = RoundMode::Practice;
OperationSet operations = OperationSet::AddSubtract;
std::string settingsReturn = "home";
bool timerHidden = false;
bool neutralDefeat = false;
bool clockRunning = false;
double taskStart = 0.0;

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

std::string eventValue(const val& event) {
    return event["currentTarget"]["value"].as<std::string>();
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

std::string formatFactor(double factor) {
    const auto hundredths = std::lround(factor * 100);
    const auto fraction = hundredths % 100;
    return std::to_string(hundredths / 100) + "," + (fraction < 10 ? "0" : "") + std::to_string(fraction);
}

const char* operationLabel(OperationSet choice) {
    return operationLabels[static_cast<int>(choice)];
}

std::string recordKey(NumberRange recordRange, RoundMode mode, OperationSet choice) {
    return "rechentrainer.record." + std::to_string(static_cast<int>(recordRange))
        + "." + std::to_string(static_cast<int>(mode))
        + "." + std::to_string(static_cast<int>(choice));
}

void saveRecords() {
    const auto& stats = session->stats();
    const auto key = recordKey(range, roundMode, operations);
    const auto scoreKey = key + ".score";
    const auto streakKey = key + ".streak";
    if (stats.score > readStoredNumber(scoreKey.c_str())) {
        writeStoredNumber(scoreKey.c_str(), static_cast<double>(stats.score));
    }
    if (stats.bestStreak > readStoredNumber(streakKey.c_str())) {
        writeStoredNumber(streakKey.c_str(), stats.bestStreak);
    }
}

void renderRecords() {
    const auto selectedMode = element("record-mode")["value"].as<std::string>() == "endless"
        ? RoundMode::Endless : RoundMode::Practice;
    const auto selectedOperations = parseInt(element("record-operations")["value"].as<std::string>());
    if (!selectedOperations || *selectedOperations < 0 || *selectedOperations >= static_cast<int>(operationLabels.size())) {
        return;
    }
    for (const auto recordRange : ranges) {
        const auto key = recordKey(recordRange, selectedMode, static_cast<OperationSet>(*selectedOperations));
        const auto score = static_cast<std::int64_t>(readStoredNumber((key + ".score").c_str()));
        const auto streak = static_cast<std::int64_t>(readStoredNumber((key + ".streak").c_str()));
        const auto limit = std::to_string(static_cast<int>(recordRange));
        setText(("record-score-" + limit).c_str(), formatPoints(score));
        setText(("record-streak-" + limit).c_str(), std::to_string(streak));
    }
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
    const auto limit = std::to_string(static_cast<int>(range));
    setText("setup-operations", operationLabel(operations));
    for (const auto candidate : ranges) {
        const auto id = "range-" + std::to_string(static_cast<int>(candidate));
        setAttribute(element(id.c_str()), "aria-pressed", candidate == range ? "true" : "false");
    }
    const bool isEndless = roundMode == RoundMode::Endless;
    setAttribute(element("mode-practice"), "aria-pressed", isEndless ? "false" : "true");
    setAttribute(element("mode-endless"), "aria-pressed", isEndless ? "true" : "false");
    setHidden("unlock-note", !isEndless || operations != OperationSet::All);
    setHidden("endless-note", !isEndless);
    setText("range-label", "Bis " + limit);
}

void updateClock(double seconds) {
    if (timerHidden) {
        return;
    }
    const auto tenths = std::lround(seconds * 10);
    setText("timer", std::to_string(tenths / 10) + "," + std::to_string(tenths % 10) + " s");
    setText("time-factor", "×" + formatFactor(timeMultiplier(seconds)));
}

void tickClock(void*) {
    if (clockRunning) {
        updateClock((emscripten_get_now() - taskStart) / 1000.0);
    }
}

void updateHud() {
    const auto& stats = session->stats();
    setText("live-score", formatPoints(stats.score));
    setText("live-multiplier", "×" + formatFactor(streakMultiplier(stats)));
    setText("live-streak", std::to_string(stats.streak));
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
}

void showQuestion() {
    const auto& task = session->currentTask();
    const bool killer = session->currentKind() == TaskKind::Killer;
    const bool endless = roundMode == RoundMode::Endless;
    const int number = session->questionNumber();
    const int phase = (number - 1) % 10 + 1;
    setAttribute(element("game"), "data-killer", killer ? "true" : "false");
    setHidden("killer-title", !killer);
    setHidden("progress-track", !endless);
    setHidden("progress-count", !endless);
    element("progress-track").set("value", phase);
    setText("progress-count", std::to_string(phase) + " / 10");
    setText("question-label", "Aufgabe " + std::to_string(number));
    renderExpression(task.expression);
    setFeedback("", "");
    updateHud();

    auto answer = element("answer");
    answer.set("value", std::string());
    answer.set("readOnly", false);
    setAttribute(answer, "inputmode", task.allowsDecimal ? "decimal" : "numeric");
    answer.call<void>("removeAttribute", std::string("aria-invalid"));
    setHidden("decimal-hint", !task.allowsDecimal);
    setHidden("check-button", false);
    setHidden("next-button", true);
    setHidden("timer-panel", timerHidden);
    taskStart = emscripten_get_now();
    clockRunning = true;
    updateClock(0.0);
    answer.call<void>("focus");
}

void showResults() {
    clockRunning = false;
    const auto& stats = session->stats();
    setText("result-mode", std::string(operationLabel(operations)) + " · Bis " +
        std::to_string(static_cast<int>(range)));
    setText("result-correct", std::to_string(stats.correct) + " / " + std::to_string(stats.answered));
    setText("result-streak", std::to_string(stats.bestStreak));
    setText("result-score", formatPoints(stats.score));
    showScreen("results");
}

void handleNavigation(val event) {
    const std::string target = eventValue(event);
    if (target == "settings") {
        settingsReturn = element("setup")["hidden"].as<bool>() ? "home" : "setup";
    }
    if (target == "records") {
        renderRecords();
    }
    if (target == "home") {
        clockRunning = false;
        session.reset();
    }
    updateSelection();
    showScreen(target);
}

void handleSettingsBack(val) {
    updateSelection();
    showScreen(settingsReturn);
}

void handleOperationChoice(val event) {
    const auto value = parseInt(eventValue(event));
    if (value && *value >= 0 && *value < static_cast<int>(operationLabels.size())) {
        operations = static_cast<OperationSet>(*value);
        writeStoredNumber("rechentrainer.operations", *value + 1);
        element("record-operations").set("value", std::to_string(*value));
    }
}

void handlePreferenceChange(val) {
    timerHidden = element("hide-timer")["checked"].as<bool>();
    neutralDefeat = element("neutral-defeat")["checked"].as<bool>();
    writeStoredNumber("rechentrainer.hide-timer", timerHidden);
    writeStoredNumber("rechentrainer.neutral-defeat", neutralDefeat);
}

void handleRangeChoice(val event) {
    const auto value = parseInt(eventValue(event));
    if (!value) {
        return;
    }
    if (const auto selected = selectNumberRange(*value)) {
        range = *selected;
        updateSelection();
    }
}

void handleModeChoice(val event) {
    roundMode = eventValue(event) == "endless" ? RoundMode::Endless : RoundMode::Practice;
    updateSelection();
}

void handleRecordFilter(val) {
    renderRecords();
}

void handleStart(val) {
    session.emplace(range, roundMode, operations);
    showScreen("practice");
    showQuestion();
}

void handleAnswerSubmit(val event) {
    event.call<void>("preventDefault");
    if (!session || !clockRunning) {
        return;
    }

    auto input = element("answer");
    const double seconds = (emscripten_get_now() - taskStart) / 1000.0;
    const auto previousScore = session->stats().score;
    const auto answer = session->submit(input["value"].as<std::string>(), seconds);
    if (!answer) {
        return;
    }

    const auto& task = session->currentTask();
    if (*answer == AnswerStatus::Invalid) {
        setFeedback(task.allowsDecimal ? "Gib eine Zahl mit höchstens einer Nachkommastelle ein."
                                      : "Gib eine ganze Zahl ein.", "invalid");
        setAttribute(input, "aria-invalid", "true");
        input.call<void>("focus");
        input.call<void>("select");
        return;
    }

    clockRunning = false;
    updateClock(seconds);
    updateHud();
    saveRecords();
    const auto solution = formatAnswer(task.answerTenths);
    if (session->isDefeated()) {
        setText("defeat-title", neutralDefeat ? "Runde beendet" : "Du bist gestorben");
        setText("defeat-solution", task.expression + " = " + solution);
        showScreen("defeat");
        return;
    }

    input.call<void>("removeAttribute", std::string("aria-invalid"));
    input.set("readOnly", true);
    if (*answer == AnswerStatus::Correct) {
        const auto points = formatPoints(session->stats().score - previousScore);
        setFeedback("Richtig! +" + points + " Punkte", "correct");
    } else {
        setFeedback("Die richtige Lösung ist " + solution + ".", "incorrect");
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
    if (session) {
        showResults();
    }
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
    emscripten::function("handleSettingsBack", &handleSettingsBack);
    emscripten::function("handleOperationChoice", &handleOperationChoice);
    emscripten::function("handlePreferenceChange", &handlePreferenceChange);
    emscripten::function("handleRangeChoice", &handleRangeChoice);
    emscripten::function("handleModeChoice", &handleModeChoice);
    emscripten::function("handleRecordFilter", &handleRecordFilter);
    emscripten::function("handleStart", &handleStart);
    emscripten::function("handleAnswerSubmit", &handleAnswerSubmit);
    emscripten::function("handleNext", &handleNext);
    emscripten::function("handleEnd", &handleEnd);
}

int main() {
    const double stored = readStoredNumber("rechentrainer.operations");
    if (stored >= 1 && stored <= operationLabels.size()) {
        operations = static_cast<OperationSet>(static_cast<int>(stored) - 1);
    }
    timerHidden = readStoredNumber("rechentrainer.hide-timer") != 0;
    neutralDefeat = readStoredNumber("rechentrainer.neutral-defeat") != 0;

    const auto operationId = "operations-" + std::to_string(static_cast<int>(operations));
    element(operationId.c_str()).set("checked", true);
    element("record-operations").set("value", std::to_string(static_cast<int>(operations)));
    element("hide-timer").set("checked", timerHidden);
    element("neutral-defeat").set("checked", neutralDefeat);

    bindGroup("[data-nav]", "onclick", "handleNavigation");
    bindGroup("[data-range]", "onclick", "handleRangeChoice");
    bindGroup("[data-mode]", "onclick", "handleModeChoice");
    bindGroup("[data-start]", "onclick", "handleStart");
    bindGroup("[data-end]", "onclick", "handleEnd");
    bindGroup("[name=operations]", "onchange", "handleOperationChoice");
    bindGroup("[data-preference]", "onchange", "handlePreferenceChange");
    bindGroup("[data-record-filter]", "onchange", "handleRecordFilter");
    element("settings-back").set("onclick", val::module_property("handleSettingsBack"));
    element("answer-form").set("onsubmit", val::module_property("handleAnswerSubmit"));
    element("next-button").set("onclick", val::module_property("handleNext"));
    element("home-actions").set("disabled", false);
    setHidden("loading-status", true);
    updateSelection();
    emscripten_set_interval(tickClock, 125.0, nullptr);
}
