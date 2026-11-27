#include "../src/practice_session.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/eventloop.h>
#include <emscripten/val.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

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

EM_JS(emscripten::EM_VAL, readEndlessRuns, (), {
    const runs = [];
    const legacyKey = "rechentrainer.record.endless";
    const isCount = value => Number.isSafeInteger(value) && value >= 0;
    try {
        for (const key of Object.keys(localStorage)) {
            if (key !== legacyKey && !key.startsWith("rechentrainer.run.endless.")) continue;
            try {
                const run = JSON.parse(localStorage.getItem(key));
                if (!run || !isCount(run.score) || !isCount(run.streak) || run.streak > 2147483647) continue;
                if (key !== legacyKey && (!isCount(run.duration) || !isCount(run.highestMultiplier)
                    || !isCount(run.killerWins) || run.killerWins > run.streak)) continue;
                runs.push({ ...run, key });
            } catch {
                // Skip a damaged entry and keep reading the other runs.
            }
        }
    } catch {
        // The browser can block storage.
    }
    return Emval.toHandle(runs);
});

EM_JS(bool, writeEndlessRun, (const char* key, double score, int streak, double duration,
    double highestMultiplier, int killerWins), {
    try {
        localStorage.setItem(UTF8ToString(key), JSON.stringify({ score, streak, duration, highestMultiplier, killerWins }));
        return true;
    } catch {
        return false;
    }
});

EM_JS(void, removeEndlessRun, (const char* key), {
    try {
        localStorage.removeItem(UTF8ToString(key));
    } catch {
        // The leaderboard still displays at most five entries.
    }
});

namespace {

using emscripten::val;

constexpr std::array ranges{NumberRange::UpTo10, NumberRange::UpTo100, NumberRange::UpTo1000};
constexpr std::array screens{"home", "setup", "info", "records", "practice", "defeat", "results"};
constexpr std::array operationChoices{
    OperationMask{1}, OperationMask{2}, OperationMask{4}, OperationMask{8}
};
constexpr std::array legacyOperationChoices{
    OperationMask{1}, OperationMask{2}, OperationMask{4}, OperationMask{8},
    additionAndSubtraction, OperationMask{12}, allOperations
};

constexpr std::size_t leaderboardSize = 5;
constexpr auto legacyRecordKey = "rechentrainer.record.endless";

struct RunDetails {
    std::int64_t duration;
    std::int64_t highestMultiplier;
    int killerWins;
};

struct RunRecord {
    std::string key;
    std::int64_t score;
    int streak;
    std::optional<RunDetails> details;
};

std::optional<PracticeSession> session;
NumberRange range = NumberRange::UpTo100;
OperationMask practiceOperations = additionAndSubtraction;
int decimalPlaces = 1;
bool clockRunning = false;
double taskStart = 0.0;
double roundStart = 0.0;
bool roundFinished = false;
std::vector<RunRecord> temporaryRuns;

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

void focusWithoutScroll(const val& target) {
    auto options = val::object();
    options.set("preventScroll", true);
    target.call<void>("focus", options);
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

std::string twoDigits(std::int64_t value) {
    return (value < 10 ? "0" : "") + std::to_string(value);
}

std::string formatDuration(std::int64_t seconds) {
    const auto minutes = seconds / 60;
    if (minutes >= 60) {
        return std::to_string(minutes / 60) + ":" + twoDigits(minutes % 60) + ":" + twoDigits(seconds % 60);
    }
    return std::to_string(minutes) + ":" + twoDigits(seconds % 60);
}

std::vector<RunRecord> storedRuns() {
    const auto entries = val::take_ownership(readEndlessRuns());
    const auto count = entries["length"].as<unsigned>();
    std::vector<RunRecord> runs;
    runs.reserve(count);
    for (unsigned index = 0; index < count; ++index) {
        const auto entry = entries[index];
        RunRecord run{entry["key"].as<std::string>(), static_cast<std::int64_t>(entry["score"].as<double>()),
                      entry["streak"].as<int>(), std::nullopt};
        if (run.key != legacyRecordKey) {
            run.details = RunDetails{static_cast<std::int64_t>(entry["duration"].as<double>()),
                                     static_cast<std::int64_t>(entry["highestMultiplier"].as<double>()),
                                     entry["killerWins"].as<int>()};
        }
        runs.push_back(std::move(run));
    }
    return runs;
}

void rankRuns(std::vector<RunRecord>& runs) {
    std::sort(runs.begin(), runs.end(), [](const RunRecord& left, const RunRecord& right) {
        if (left.score != right.score) return left.score > right.score;
        if (left.streak != right.streak) return left.streak > right.streak;
        return left.key < right.key;
    });
}

void finishRound(double finishedAt) {
    if (roundFinished) {
        return;
    }
    roundFinished = true;
    const auto& stats = session->stats();
    if (!session->isEndless() || stats.answered == 0) {
        return;
    }

    const auto duration = static_cast<std::int64_t>(std::max(0.0, finishedAt - roundStart) / 1000.0);
    const auto timestamp = val::global("Date").call<double>("now");
    std::random_device random;
    const auto key = "rechentrainer.run.endless." + std::to_string(static_cast<std::int64_t>(timestamp)) + "."
        + std::to_string(random()) + "." + std::to_string(random());
    if (!writeEndlessRun(key.c_str(), stats.score, stats.bestStreak, duration, stats.highestMultiplier, stats.killerWins)) {
        temporaryRuns.push_back({key, stats.score, stats.bestStreak, RunDetails{duration, stats.highestMultiplier, stats.killerWins}});
        rankRuns(temporaryRuns);
        if (temporaryRuns.size() > leaderboardSize) temporaryRuns.resize(leaderboardSize);
        return;
    }

    // Each run has its own key, so browser tabs can save independently.
    auto runs = storedRuns();
    rankRuns(runs);
    for (auto index = leaderboardSize; index < runs.size(); ++index) {
        if (runs[index].key != legacyRecordKey) removeEndlessRun(runs[index].key.c_str());
    }
}

void renderRecords() {
    auto runs = storedRuns();
    runs.insert(runs.end(), temporaryRuns.begin(), temporaryRuns.end());
    rankRuns(runs);
    setHidden("records-empty", !runs.empty());
    auto list = element("records-list");
    list.set("textContent", std::string());
    const auto card = element("record-template")["content"]["firstElementChild"];
    for (std::size_t index = 0; index < std::min(leaderboardSize, runs.size()); ++index) {
        const auto& run = runs[index];
        auto row = card.call<val>("cloneNode", true);
        const auto set = [&row](const char* selector, const std::string& value) {
            row.call<val>("querySelector", std::string(selector)).set("textContent", value);
        };
        set("[data-rank]", "#" + std::to_string(index + 1));
        set("[data-score]", formatPoints(run.score));
        set("[data-streak]", formatPoints(run.streak));
        set("[data-duration]", run.details ? formatDuration(run.details->duration) : "–");
        set("[data-multiplier]", run.details ? "×" + formatFactor(run.details->highestMultiplier) : "–");
        set("[data-killers]", run.details ? formatPoints(run.details->killerWins) : "–");
        list.call<val>("appendChild", row);
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

void showDefeat(double finishedAt) {
    clockRunning = false;
    finishRound(session->hasTimedOut() ? taskStart + killerTimeLimit * 1000.0 : finishedAt);
    const auto& task = session->currentTask();
    setHidden("defeat-timeout", !session->hasTimedOut());
    setText("defeat-solution", task.expression + " = " + formatAnswer(task.answerUnits));
    showScreen("defeat");
}

void tickClock(void*) {
    if (!clockRunning) {
        return;
    }
    const double now = emscripten_get_now();
    const double seconds = (now - taskStart) / 1000.0;
    updateClock(seconds, scoreMultiplierHundredths(session->stats(), session->currentKind(), seconds));
    if (session->expireIfNeeded(seconds)) {
        showDefeat(now);
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
    setHidden("progress-panel", !endless);
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
    focusWithoutScroll(answer);
}

void showResults() {
    clockRunning = false;
    finishRound(emscripten_get_now());
    const auto& stats = session->stats();
    const std::string modeLabel = session->isEndless() ? "Endlos · " : "Üben · ";
    const std::string points = formatPoints(stats.score);
    setText("result-mode", modeLabel + "Bis " + formatPoints(session->currentNumberLimit()));
    setText("result-points", points);
    setText("result-correct-count", formatPoints(stats.correct));
    setText("result-answered-count", formatPoints(stats.answered));
    setText("result-streak", formatPoints(stats.bestStreak));
    setText("result-score", points);
    showScreen("results");
}

void handleNavigation(val event) {
    const auto target = event["currentTarget"]["value"].as<std::string>();
    if (target == "records") {
        renderRecords();
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
    roundStart = emscripten_get_now();
    roundFinished = false;
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
    const double now = emscripten_get_now();
    const double seconds = (now - taskStart) / 1000.0;
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
        focusWithoutScroll(input);
        input.call<void>("select");
        return;
    }

    clockRunning = false;
    updateClock(seconds, *answer == AnswerStatus::Correct ? multiplier : 0);
    updateHud();
    if (session->isDefeated()) {
        showDefeat(now);
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
    focusWithoutScroll(element("next-button"));
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
    const double now = emscripten_get_now();
    if (clockRunning && session->expireIfNeeded((now - taskStart) / 1000.0)) {
        showDefeat(now);
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
    element("info-button").set("disabled", false);
    setHidden("loading-status", true);
    updateSelection();
    emscripten_set_interval(tickClock, 125.0, nullptr);
}
