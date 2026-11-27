#include <emscripten/html5.h>
#include <emscripten/val.h>

#include <string>

namespace {

emscripten::val element(const char* id) {
    return emscripten::val::global("document")
        .call<emscripten::val>("getElementById", std::string(id));
}

bool beimTestklick(int, const EmscriptenMouseEvent*, void*) {
    element("status").set("textContent", std::string("C++ läuft im Browser."));
    return true;
}

}  // namespace

int main() {
    const auto ergebnis = emscripten_set_click_callback(
        "#test-button", nullptr, false, beimTestklick);

    if (ergebnis != EMSCRIPTEN_RESULT_SUCCESS) {
        element("status").set("textContent", std::string("Der Testbutton konnte nicht gestartet werden."));
        return 1;
    }

    element("test-button").set("disabled", false);
    element("status").set("textContent", std::string("Bereit zum Testen."));
}
