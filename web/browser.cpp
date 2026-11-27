#include "../src/practice_session.hpp"
#include "../src/task_selection.hpp"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <charconv>
#include <string>
#include <system_error>

namespace {

using emscripten::val;

std::optional<PracticeSession> session;

val element(const char* id) {
    return val::global("document").call<val>("getElementById", std::string(id));
}

void setPressed(const char* id, bool pressed) {
    element(id).call<void>("setAttribute", std::string("aria-pressed"),
                           std::string(pressed ? "true" : "false"));
}

void setFeedback(const std::string& message, const std::string& state) {
    auto feedback = element("feedback");
    feedback.set("textContent", message);
    feedback.call<void>("setAttribute", std::string("data-state"), state);
}

void showQuestion() {
    auto answerInput = element("answer");
    const bool isAddition = session->operation() == Operation::Add;

    setPressed("add-button", isAddition);
    setPressed("subtract-button", !isAddition);
    element("question").set("textContent", session->currentTask().expression + " = ?");
    setFeedback("", "");

    answerInput.set("value", std::string());
    answerInput.set("readOnly", false);
    answerInput.call<void>("removeAttribute", std::string("aria-invalid"));
    element("check-button").set("hidden", false);
    element("next-button").set("hidden", true);
    answerInput.call<void>("focus");
}

void handleRangeClick(val event) {
    const auto value = event["currentTarget"]["value"].as<std::string>();
    const auto end = value.data() + value.size();
    int limit = 0;
    const auto parsed = std::from_chars(value.data(), end, limit);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return;
    }

    const auto range = selectNumberRange(limit);
    if (!range) {
        return;
    }

    session.emplace(*range);
    element("practice-title").set("textContent", "Bis " + std::to_string(limit));
    element("menu-screen").set("hidden", true);
    element("practice-screen").set("hidden", false);
    showQuestion();
}

void handleOperationClick(val event) {
    const bool isSubtraction = event["currentTarget"]["value"].as<std::string>() == "subtract";
    const auto operation = isSubtraction ? Operation::Subtract : Operation::Add;

    if (session && session->changeOperation(operation)) {
        showQuestion();
    }
}

void handleAnswerSubmit(val event) {
    event.call<void>("preventDefault");
    if (!session) {
        return;
    }

    auto answerInput = element("answer");
    const auto status = session->submit(answerInput["value"].as<std::string>());
    if (!status) {
        return;
    }

    if (*status == AnswerStatus::Invalid) {
        setFeedback("Gib eine ganze Zahl ein.", "invalid");
        answerInput.call<void>("setAttribute", std::string("aria-invalid"), std::string("true"));
        answerInput.call<void>("focus");
        answerInput.call<void>("select");
        return;
    }

    answerInput.call<void>("removeAttribute", std::string("aria-invalid"));
    if (*status == AnswerStatus::Correct) {
        setFeedback("Richtig!", "correct");
    } else {
        const auto solution = std::to_string(session->currentTask().answer);
        setFeedback("Die richtige Lösung ist " + solution + ".", "incorrect");
    }

    auto nextButton = element("next-button");
    answerInput.set("readOnly", true);
    element("check-button").set("hidden", true);
    nextButton.set("hidden", false);
    nextButton.call<void>("focus");
}

void handleNextClick(val) {
    if (session && session->nextTask()) {
        showQuestion();
    }
}

void handleBackClick(val) {
    if (!session) {
        return;
    }

    const auto limit = std::to_string(static_cast<int>(session->range()));
    session.reset();
    element("practice-screen").set("hidden", true);
    element("menu-screen").set("hidden", false);
    const auto buttonId = "range-" + limit;
    element(buttonId.c_str()).call<void>("focus");
}

}  // namespace

EMSCRIPTEN_BINDINGS(math_trainer) {
    emscripten::function("handleRangeClick", &handleRangeClick);
    emscripten::function("handleOperationClick", &handleOperationClick);
    emscripten::function("handleAnswerSubmit", &handleAnswerSubmit);
    emscripten::function("handleNextClick", &handleNextClick);
    emscripten::function("handleBackClick", &handleBackClick);
}

int main() {
    for (const auto* id : {"range-10", "range-100", "range-1000"}) {
        auto button = element(id);
        button.set("onclick", val::module_property("handleRangeClick"));
        button.set("disabled", false);
    }

    element("add-button").set("onclick", val::module_property("handleOperationClick"));
    element("subtract-button").set("onclick", val::module_property("handleOperationClick"));
    element("answer-form").set("onsubmit", val::module_property("handleAnswerSubmit"));
    element("next-button").set("onclick", val::module_property("handleNextClick"));
    element("back-button").set("onclick", val::module_property("handleBackClick"));
    element("loading-status").set("hidden", true);
}
